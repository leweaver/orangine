from subprocess import run, PIPE, Popen, TimeoutExpired
import os, sys, argparse


class Builder:
    def __init__(self, oe_root, verbose):
        self.oe_root = os.path.abspath(oe_root)
        self.verbose = verbose

        print ("oe_root=" + self.oe_root)

        # gather environment
        self.env_vctools = os.environ.get("VCToolsInstallDir")
        self.env_DevEnvDir = os.environ.get("DevEnvDir")
        self.env_target_arch = os.environ.get("VSCMD_ARG_TGT_ARCH")
        self.env_vs_version = os.environ.get("VisualStudioVersion")

        print ("env_vctools=" + self.env_vctools)
        print ("env_DevEnvDir=" + self.env_DevEnvDir)
        print ("env_target_arch=" + self.env_target_arch)
        print ("env_vs_version=" + self.env_vs_version)

        for env_var in ['VCToolsInstallDir', 'DevEnvDir', 'VSCMD_ARG_TGT_ARCH', 'VisualStudioVersion']:
            if not os.environ.get(env_var):
                raise Exception(f"{env_var} environment variable is not set. Did you run vcvars32.bat or vcvars64.bat ?")
        
        self.path_cmake = f"{self.env_DevEnvDir}COMMONEXTENSIONS/MICROSOFT/CMAKE/CMake/bin/cmake.exe"
        self.path_ninja = f"{self.env_DevEnvDir}COMMONEXTENSIONS/MICROSOFT/CMAKE/Ninja/ninja.exe".replace('/', '\\')
        
        print ("path_cmake=" + self.path_cmake)
        print ("path_ninja=" + self.path_ninja)

        for exe in [self.path_cmake, self.path_ninja]:
            if not os.path.exists(exe):
                raise Exception(f"{exe} does not exist. Did you run vcvars32.bat or vcvars64.bat, and have you installed CMake in the visual studio installer?")

    def _call(self, program, note):
        """
        Passing in as a single string, rather than the array of args, since CMake doesn't
        like how python adds quotes around everything.
        """

        if self.verbose:
            print("> "+ program)

        with Popen(program, stdout=PIPE, stderr=PIPE, shell=True, text=True) as proc:
            
            while True:
                try:
                    # This timeout needs to be long enough for the build to complete. Not sure of a better way to handle this.
                    p_stdout, p_stderr = proc.communicate(timeout=5*60)
                except TimeoutExpired:
                    print ("Timed out, killing process")
                    proc.kill()
                    p_stdout, p_stderr = proc.communicate()
                    break
                    
                if p_stdout and self.verbose:
                    print(p_stdout)
                if p_stderr and self.verbose:
                    print(p_stderr)

                if proc.poll() is not None:
                    break
                    
            rc = proc.poll()
            if 0 is not rc:
                # if we surpressed output before, it might be useful now, so print it.
                if p_stdout and not self.verbose:
                    print(p_stdout)
                if p_stderr and not self.verbose:
                    print(p_stderr)

                print("  [[31mFailed[0m] " + note)
                raise Exception("Failed to execute command , returned " + str(rc))

            print("  [[32mOK[0m] " + note)

    def build_all(self):
        # pybind11
        self.cmake_ninja_install(self.oe_root + "/thirdparty/pybind11", "pybind11", "Debug")
        self.cmake_ninja_install(self.oe_root + "/thirdparty/pybind11", "pybind11", "Release")

        # g3log
        self.cmake_ninja_install(self.oe_root + "/thirdparty/g3log", "g3log", "Debug")
        self.cmake_ninja_install(self.oe_root + "/thirdparty/g3log", "g3log", "Release")
        
        # MikktSpace and tinygltf are build using our own custom CMakeLists file.
        self.cmake_ninja_install(self.oe_root + "/thirdparty", "thirdparty", "Debug")
        self.cmake_ninja_install(self.oe_root + "/thirdparty", "thirdparty", "Release")

        # gtest and gmock
        self.cmake_msvc_shared(self.oe_root + "/thirdparty/googletest/googletest", "gtest")
        self.cmake_msvc_shared(self.oe_root + "/thirdparty/googletest/googlemock", "gmock")
        
        # DirectXTK
        self.msbuild_directxtk_sln(self.oe_root + "/thirdparty/DirectXTK")

        # Hacky Steps:
        # For some reason, the g3logger cmake find_module needs these directories to exist. Even though they are empty.
        for d in [
                '/Debug/COMPONENT',
                '/Release/COMPONENT',
                '/Debug/libraries',
                '/Release/libraries',
            ]:
            bin_path = self.oe_root + '/bin/' + self.env_target_arch + d
            if not os.path.isdir(bin_path):
                print ("Creating bin output directory " + bin_path)
                os.makedirs(bin_path)

    def _chdir_cache(self, target_name, cache_type, build_config=None):
        os.chdir(self.oe_root)

        # where should the cmake cache be created?
        path_cmake_cache_dir = f"{self.oe_root}/thirdparty/cmake-{target_name}-{cache_type}-{self.env_target_arch}"
        if build_config is not None:
            path_cmake_cache_dir = path_cmake_cache_dir + "-" + build_config

        if not os.path.exists(path_cmake_cache_dir):
            os.mkdir(path_cmake_cache_dir)
            
        os.chdir(path_cmake_cache_dir)

        path_logfile = path_cmake_cache_dir + "/oe-build-log.txt"

        if self.verbose:
            print(target_name + " build output: " + path_logfile)

        # delete the existing CMake Cache to start afresh
        path_cmake_cache = path_cmake_cache_dir + "/CMakeCache.txt" 
        if os.path.exists(path_cmake_cache):
            os.remove(path_cmake_cache)

        return path_logfile


    def cmake_ninja_install(self, source_path, target_name, build_config):
        path_logfile = self._chdir_cache(target_name=target_name, cache_type='ninjabuild', build_config=build_config)

        cmake_args = [
            '-G', 
            'Ninja',
            f'-DCMAKE_CXX_COMPILER:FILEPATH="{self.env_vctools}bin/Host{self.env_target_arch}/{self.env_target_arch}/cl.exe"',
            f'-DCMAKE_MAKE_PROGRAM="{self.path_ninja}"',
            f'-DCMAKE_INSTALL_PREFIX:PATH="{self.oe_root}/bin/{self.env_target_arch}/{build_config}"',
            f'-DCMAKE_BUILD_TYPE={build_config}',
        ]

        if build_config == "Debug":
            cmake_args.append("-DCMAKE_DEBUG_POSTFIX=_d")

        cmake_args.append(source_path)

        print(f"NINJA: {target_name} {build_config} {self.env_target_arch}")
        self._call('cmd /c ""' + self.path_cmake + '" ' + " ".join(cmake_args) + '"', f"CMake -G Ninja")
        self._call('cmd /c ""' + self.path_ninja + '" install"', "ninja install")

    def cmake_msvc_shared(self, source_path, target_name):
        path_logfile = self._chdir_cache(target_name=target_name, cache_type='msbuild', build_config=None)
        
        if self.env_vs_version == "15.0":
            make_generator = "Visual Studio 15 2017"
        elif self.env_vs_version == "16.0":
            make_generator = "Visual Studio 16 2019"
        else:
            raise Exception("Unsupported visual studio version: " + self.env_vs_version)

        make_architecture = "Win32" if self.env_target_arch == "x86" else "x64"

        cmake_args = [
            '-DBUILD_SHARED_LIBS=ON',
            '-G',
            f'"{make_generator}"',
            '-A',
            make_architecture,
            source_path
        ]

        print(f"MSVC: {target_name} {self.env_target_arch}")
        self._call('cmd /c ""' + self.path_cmake + '" ' + " ".join(cmake_args) + '"', f"CMake -G {make_generator}")

        self._call(f'msbuild {target_name}.sln /p:Configuration=Debug /p:Platform="{make_architecture}"', "msbuild Debug")
        self._call(f'msbuild {target_name}.sln /p:Configuration=Release /p:Platform="{make_architecture}"', "msbuild Release")

    def msbuild_directxtk_sln(self, source_path):
        os.chdir(source_path)
        
        if self.env_vs_version == "15.0":
            sln = "DirectXTK_Desktop_2017_Win10"
        elif self.env_vs_version == "16.0":
            sln = "DirectXTK_Desktop_2019_Win10"
        else:
            raise Exception("Unsupported visual studio version: " + self.env_vs_version)

        make_architecture = "Win32" if self.env_target_arch == "x86" else "x64"

        print(f"DirectXTK: {self.env_target_arch}")
        self._call(f'msbuild {sln}.sln /p:Configuration=Debug /p:Platform="{make_architecture}"', "msbuild Debug")
        self._call(f'msbuild {sln}.sln /p:Configuration=Release /p:Platform="{make_architecture}"', "msbuild Release")

if __name__ == "__main__":
    
    parser = argparse.ArgumentParser()
    parser.add_argument("--verbose", help="increase output verbosity")
    args = parser.parse_args()

    builder = Builder(os.path.curdir, args.verbose)
    builder.build_all()