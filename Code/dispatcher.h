// The following code is by Andrew Potter (agpotter@gmail.com), from
// gtkmm-list@gnome.org.

#include <queue>

class CallbackDispatcher {
public:
    CallbackDispatcher() {
        dispatcher.connect(sigc::mem_fun(this,
	    &CallbackDispatcher::on_dispatch));
    }

    typedef sigc::slot<void> Message;
    void send(Message msg) {
        Glib::RecMutex::Lock lock(mutex);
        queue.push(msg);
        dispatcher();
    }

private:
    /* CallbackDispatcher may not be copied, so we must hide these
     * constructors. */
    CallbackDispatcher(const CallbackDispatcher&);
    CallbackDispatcher& operator=(const CallbackDispatcher&);

    Glib::RecMutex mutex;
    std::queue<Message> queue;
    Glib::Dispatcher dispatcher;

    void on_dispatch() {
        Glib::RecMutex::Lock lock(mutex);
        while (!queue.empty()) {
            queue.front()();
            queue.pop();
        }
    }
};
