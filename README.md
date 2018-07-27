# First time setup
I prefer to use bash shell for git commands. (the ubuntu shell within windows, or git-bash work equally well).

## OS & Visual Studio Version
You must have Windows 10 (RS4 or later), with Visual Studio 2017 15.7.5, or later (Windows SDK 10.0.17134.0 or later)

## Get the code & dependencies from GIT
1. Install ssh keypair to your system (run `ssh-keygen`), and load the public key to your github account
1. Clone the repository.
1. Download thirdparty dependencies. In the root repository directory, run: 

```
git submodule update --init --recursive
```

_Note_, to update the submodules to the latest version pointed to by the repository, run the following command:
```
git pull --recurse-submodules
git submodule update --recursive --remote
```

## Build dependencies
### g3log
We need to create the appropriate visual studio projects, using CMake.
1. Using the start menu, find and open `Developer Command Prompt for VS 2017`.
1. Change directory to the root of the GIT repository
1. Run:

```
.\create-thirdparty-projects.bat
```

### Visual Studio 2017

# Renderer 
## Normals and Tangents
Normals are expected to be in tangent space. Further, the tangents on meshes must be generated using the MikkiT algorithm.

Tangents in art assets must be generated using the MikkiT algorithm (see [Simulation of Wrinkled Surfaces Revisited by Morten S. Mikkelsen](http://image.diku.dk/projects/media/morten.mikkelsen.08.pdf).)

# Coding Style Guide
Using [Cpp Core Guidelines](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md) with some additions/clarifications:

## Forward Declarations
Don't use for model classes in 'Manager' class header files (ie, for the things that the manager class managers)
Do use when referencing manager classes in header files

## Pointers, References, SmartPointers
Function Arguments:
	[https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rr-smartptrparam](R.30)
	Prefer references to objects, but take a pointer if it can be null.
	Only take in a smart/unique pointer when lifetime/ownership needs to be managed:
		[https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Rr-smartptrparam](R.36)
		```
		void share(shared_ptr<widget>);            // share -- "will" retain refcount
		void reseat(shared_ptr<widget>&);          // "might" reseat ptr
		void may_share(const shared_ptr<widget>&); // "might" retain refcount
		```

## Naming
[https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#nl10-prefer-underscore_style-names](NL.10) Prefer	underscore_case for names
[https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#nl17-use-kr-derived-layout](NL.17) use KR derived layout

namespace: lower_underscored
classes, structs, enums: My_class_name
template parameters: TMy_class_name

private field members: _lowerCamelCase
public field members: lowerCamelCase
