#pragma once

#include <functional>

namespace oe {
template<typename... TEventArgs> class Dispatcher {
 public:
  using Listener = std::function<void(TEventArgs...)>;

  class ListenerId {
   public:
    ListenerId()
        : _valid(false)
    {}

   private:
    explicit ListenerId(typename std::list<Listener>::const_iterator iter)
        : _valid(true)
        , _iter(iter)
    {}

    friend Dispatcher;
    bool _valid;
    typename std::list<Listener>::const_iterator _iter;
  };

  ListenerId addListener(Listener listener)
  {
    return ListenerId(_listeners.insert(_listeners.end(), listener));
  }
  void removeListener(ListenerId& listenerId)
  {
    if (listenerId._valid) {
      _listeners.erase(listenerId._iter);
      listenerId._valid = false;
    }
  }

 protected:
  void invokeListeners(TEventArgs... args)
  {
    for (auto& listener : _listeners) {
      listener(args...);
    }
  }

 private:
  std::list<Listener> _listeners;
};

template<typename... TEventArgs> class Invokable_dispatcher : public Dispatcher<TEventArgs...> {
 public:
  void invoke(TEventArgs... args)
  {
    invokeListeners(args...);
  }
};

/**
 * Helper class that will subscribe a listener to the given dispatcher on construction; and unsubscribe from the
 * dispatcher on destruction. The Dispatcher must exist for the entire life of this scoped listener.
 * @tparam TEventArgs Argument types of the dispatcher.
 */
template<typename... TEventArgs> class ScopedListener {
 public:
  using Listener = std::function<void(TEventArgs...)>;

  ScopedListener(Dispatcher<TEventArgs...>& dispatcher, Listener listener)
      : _dispatcher(dispatcher)
      , _listenerId(dispatcher.addListener(listener))
  {}
  ~ScopedListener()
  {
    _dispatcher.removeListener(_listenerId);
  }

 private:
  Dispatcher<TEventArgs...>& _dispatcher;
  typename Dispatcher<TEventArgs...>::ListenerId _listenerId;
};

}// namespace oe
