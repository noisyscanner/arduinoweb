// 2, 3, 4, 5, 6, 7
bool lightStatus[] = {false, false, false, false, false, false};

void updateLights(int pin, bool on) {
    if (pin >= 2 && pin < 8) {
        digitalWrite(pin, on ? HIGH : LOW);
        lightStatus[pin - 2] = on;
    }
}

bool lightsAreOn(int pin) { return lightStatus[pin - 2]; }

char* lightStatusHuman(int pin) {
    char out[4];
    switch (pin) {
        case 2:
            strcpy(out, "bed");
            break;
        case 7:
            strcpy(out, "tv");
            break;
        default:
            return "";
    }
    char* ret;
    ret = malloc(sizeof(char) * 16);
    sprintf(ret, "%s %s", out, lightsAreOn(pin) ? "on" : "off");
    return ret;
}

void initPins() {
    for (int i = 2; i <= 7; i++) {
        if (i == 4) continue;
        pinMode(i, OUTPUT);
        updateLights(i, lightsAreOn(i));
    }
}

