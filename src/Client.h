#pragma once

class Client {
public:
    static Client& getInstance() { static Client inst; return inst; }

    void init();
    void shutdown();
    bool isRunning() const { return running; }

private:
    Client() = default;
    bool running = false;
};
