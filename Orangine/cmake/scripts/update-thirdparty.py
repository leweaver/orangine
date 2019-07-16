import subprocess
import os
import io
import json

def git_call(*args):
    print("> git " + " ".join(list(args)))
    return subprocess.check_call(['git'] + list(args))

def git_get_string(*args):
    print("> git " + " ".join(list(args)))
    return subprocess.check_output(['git'] + list(args)).decode("utf-8")

tp_libs = json.load(open("thirdparty.json"))

basePath =  os.path.abspath(os.curdir + "/thirdparty/")
for k, v in tp_libs.items():
    print(k, v['url'], v['ref'])
    os.chdir(basePath)
    gitPath = k + "/.git"
    if os.path.isdir(gitPath):
        os.chdir(basePath + "/" + k)
        # nuke everything and force us to be where we need to be.
        # not efficient, but bleh!
        git_call('reset', '--hard')
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

