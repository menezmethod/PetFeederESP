#pragma once

class PowerManager {
public:
    static void init();

    static void update();

    static void wakeUp();

private:
    static bool shouldEnterDeepSleep();

    static bool shouldEnterLightSleep();

    static void enterDeepSleep();

    static void enterLightSleep();
};
