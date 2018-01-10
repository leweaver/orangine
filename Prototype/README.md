# First time setup
I prefer to use bash shell for git commands. (the ubuntu shell within windows, or git-bash work equally well).

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

# Coding Style Guide
## Forward Declarations
Don't use for model classes in 'Manager' class header files (ie, for the things that the manager class managers)
Do use when referencing manager classes in header files

## Pointers vs References
Use reference wherever you can, pointers wherever you must. Avoid pointers until you can't.

Prefer references on Public facing API; pointers typically appear in the implementation.

When passing in a parameter that may not have a value, consider passing in a std::optional<T &>.


## 
classes, enums: MyClassName
template parameters: TMyClassName

private field members: m_lowerCamelCase
public field members: lowerCamelCase
