import subprocess
import os
import io
import json
import urllib.request
import tempfile
import zipfile

def git_call(*args):
    print("> git " + " ".join(list(args)))
    return subprocess.check_call(['git'] + list(args))

def git_get_string(*args):
    print("> git " + " ".join(list(args)))
    return subprocess.check_output(['git'] + list(args)).decode("utf-8")

tp_libs = json.load(open("thirdparty.json"))

basePath =  os.path.abspath(os.curdir + "/thirdparty/")
for k, v in tp_libs.items():
    repo_type = v.setdefault('type', 'git')
    print(repo_type, k, v['url'], v.setdefault('ref', ''))
    if repo_type == 'downloadzip':
        targetPath = basePath + "/" + k
        if os.path.isdir(targetPath):
            print('Already downloaded, skipping')
            continue

        tmpDir = tempfile.TemporaryDirectory()
        try:
            tmpFileName = tmpDir.name + "/" + k + ".zip"
            print('Downloading to: ' + tmpFileName)
            urllib.request.urlretrieve(v['url'], tmpFileName)
            
            print('Extracting to: ' + targetPath)
            with zipfile.ZipFile(tmpFileName, 'r') as zip_ref:
                zip_ref.extractall(targetPath)
        finally:
            print('Cleaning up temp files')
            tmpDir.cleanup()
    elif repo_type == 'git':
        os.chdir(basePath)
        gitPath = k + "/.git"
        if os.path.isdir(gitPath):
            os.chdir(basePath + "/" + k)
            # nuke everything and force us to be where we need to be.
            # not efficient, but bleh!
            git_call('reset', '--hard')
            git_call('fetch')
            git_call('checkout', v['ref'])
            status = git_get_string('status')
            if status.find("HEAD detached at") == -1:
                git_call('pull', '--ff-only')
        else:
            # just do a clone
            if v['ref'].find('refs/tags') != -1:
                git_call('clone', '--single-branch', '--branch', v['ref'], v['url'], basePath + "/" + k)
            else:
                git_call('clone', v['url'], basePath + "/" + k)
                os.chdir(basePath + "/" + k)
                git_call('checkout', v['ref'])

