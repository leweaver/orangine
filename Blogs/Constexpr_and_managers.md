# Improving manager lists using Tuples and Constexpr

Previously on the top level `Scene`, I was just keeping a field for each `Manager_base` derived manager class. This works well for simplicity, but as the number of managers grows this quickly becomes yucky and error prone (see the coding error in the `tick` method):

```cpp
class Manager_base {
public:
    virtual void initialize() = 0;
    virtual void tick() = 0;
    virtual void shutdown() = 0;
};

class Scene {
public:   
    Scene(/* ... */);
    void initialize();
    void tick();
    void shutdown();

private:
    std::unique_ptr<Scene_graph_manager> _sceneGraphManager;
    std::unique_ptr<Entity_render_manager> _sceneGraphManager;
    std::unique_ptr<Entity_scripting_manager> _entityScriptingManager;
};

void Scene::Scene(/* ... */)
{
    _sceneGraphManager = std::make_unique<Scene_graph_manager>(/* ... */);
    _entityRenderManager = std::make_unique<Entity_render_manager>(/* ... */);
    _entityScriptingManager = std::make_unique<Entity_scripting_manager>(/* ... */);
}

void Scene::initialize()
{
    // Eww - we just broke the DRY principle!
    _sceneGraphManager->initialize();
    _entityRenderManager->initialize();
    _entityScriptingManager->initialize();
}

void Scene::tick(DX::StepTimer const& timer) 
{
    _sceneGraphManager->tick();
    _entityRenderManager->tick();
    _entityScriptingManager->tick();
}

void Scene::shutdown()
{    
    _sceneGraphManager->shutdown();
    _entityRenderManager->shutdown();
    // BUG! Forgot to shutdown the entityScriptingManager!
}
```

# Simple approach: Using a vector
Since all of the managers derive from a single class, one approach is to store all of the instances in a vector of `unique_ptr<Manager_base>`. At runtime, simple iterate over the list and call a virtual tick method:

```cpp

class Scene {
    std::vector<std::unique_ptr<Manager_base>> _managers;
};

void Scene::tick(DX::StepTimer const& timer) 
{
    for (auto& manager : _managers) {
        manager->tick();
    }
}
```

Some drawbacks: 

- each call to `tick` is a v-table lookup
- not all managers actually need to be ticked, so calling their tick() method will be a runtime v-table lookup for a no-op method.
- no longer have pointers to the derived class implementations in the vector, so need to perform RTTI if even we want a derived class.

We can surely do better!

# Tuples
Tuples are great for storing a set of different types that is known at compile time, especially so when no elements share the same type:

```cpp
class Scene {
    std::tuple<
        std::unique_ptr<Scene_graph_manager>,
        std::unique_ptr<Entity_render_manager>,
        std::unique_ptr<Entity_scripting_manager>
    > _managers;
};
```

Members of a tuple can be accessed using the `std::get<T>` method, which is `constexpr`. `T` can either be an integer index into the tuple, or better, an actual member type within the tuple (only valid in tuples that do not contain more than one element of type `T`). For our example, the following are equivalent:

```cpp
// Get the tuple element from _managers at index 0
std::unique_ptr<Scene_graph_manager>& managerByIndex = std::get<0>(_managers);

// Get the tuple element from _managers matching type
std::unique_ptr<Scene_graph_manager>& managerByType  = std::get<std::unique_ptr<Scene_graph_manager>>(_managers);
```

Iterating over all elements of a tuple can be done at compile time using templates and the new `if constexpr` syntax. The great thing about this is that the `tick` method no longer needs to be virtual, since we have the actual derived types:

```cpp
class Manager_base {
public:
    // Implementing classes must declare the following public methods:
    //   void initialize();
    //   void tick();
    //   void shutdown();
};

class Scene {
    std::tuple<
        std::unique_ptr<Scene_graph_manager>,
        std::unique_ptr<Entity_render_manager>,
        std::unique_ptr<Entity_scripting_manager>
    > _managers;

    template<int TIdx>
    void tick() 
    {
        // Get the manager and call the tick method.
        std::get<TIdx>(_managers)->tick();

        // Recursively iterate to the next tuple index
        if constexpr (TIdx < std::tuple_size_v<decltype(_managers)>)
            tick<TIdx + 1>();
    }
}
```

### By the way... `if constexpr` and `decltype`
Perform the loop condition at the _end_, prior to iterating in. This is due to a peculiarity:  `decltype` is evaluated within even within an `if constexpr` expression that resolves to false. This causes `get<TIdx>` calls to access out-of-range tuple elements:

```cpp
if constexpr (TIdx < tuple_size_v<decltype(_managers)>) {
    // The decltype is still evaluated even though the condition is false, causing a compile error:
    // error C2338: tuple index out of bounds 
    static_assert(!is_same_v<void, decltype(std::get<INT_MAX>(_managers))>);
}
```

# Type relationships
In the previous section, we successfully iterate over all of the managers at compile time, calling the tick method. We can improve on this by only calling `tick` on managers that actually need it by splitting the  `Manager_base` class into a number of abstract interfaces per feature, then use `std::is_base_of_v` to determine if the current manager implements that interface.

> Remember to use the `std::remove_pointer_t` method, not a dereference operator, when providing arguments to  the STL traits methods. 

```cpp
class Manager_tickable {
    // Classes that implement this interface must have the following methods:
    //     void tick();
};

class Manager_base {
public:    
    // Classes that implement this interface must have the following methods:
    //     void initialize();
    //     void shutdown();
};

class Scene {
    std::tuple<
        std::unique_ptr<Scene_graph_manager>,
        std::unique_ptr<Entity_render_manager>,
        std::unique_ptr<Entity_scripting_manager>
    > _managers;

    template<int TIdx>
    void tick() 
    {
        using namespace std;

        // Tick only types that derive from Manager_base
        if constexpr (is_base_of_v<Manager_tickable, remove_pointer_t<decltype(get<TIdx>(_managers).get())>>)
            get<TIdx>(_managers)->tick();

        // Recursively iterate to the next tuple index
        if constexpr (TIdx + 1 < tuple_size_v<decltype(_managers)>)
            tick<TIdx + 1>();
    }
}
```

## Debugging
It can be hard to tell which code paths are being hit. Forcing a compiler warning can be one way at least see which iteration is being hit:

```cpp
template<int TIdx = 0>
constexpr void tickManagers()
{
    // Force the compiler to output some trace messages
    if constexpr (TIdx % 2 == 0) {
        int isEven;
        isEven = isEven;
    }
    else {
        int isOdd;
        isOdd = isOdd;
    }

    if constexpr (TIdx < 5) {
        tickManagers<TIdx + 1>();
    }
}
```

Build Output:
```log
1>------ Build started: Project: Prototype, Configuration: Debug x64 ------
1>Scene.cpp
1>c:\repos\orangine\prototype\scene.h(119): warning C4700: uninitialized local variable 'isEven' used
1>c:\repos\orangine\prototype\scene.h(123): warning C4700: uninitialized local variable 'isOdd' used
1>c:\repos\orangine\prototype\scene.h(119): warning C4700: uninitialized local variable 'isEven' used
1>c:\repos\orangine\prototype\scene.h(123): warning C4700: uninitialized local variable 'isOdd' used
1>c:\repos\orangine\prototype\scene.h(119): warning C4700: uninitialized local variable 'isEven' used
1>c:\repos\orangine\prototype\scene.h(123): warning C4700: uninitialized local variable 'isOdd' used
```