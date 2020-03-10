#include <Ethernet.h>
#include <SD.h>
#include <SPI.h>

const char* fileName = "index.htm";
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 121);
EthernetServer server(80);

EthernetClient client;
File pageFile;

boolean headWritten = false;

void setup() {
    Serial.begin(9600);

    loadHTML();
    Ethernet.begin(mac, ip);
    server.begin();
    Serial.println(Ethernet.localIP());
    initPins();
}

void loadHTML() {
    pinMode(4, OUTPUT);
    while (!SD.begin(4)) {
        ;
    }

    if (!SD.exists(fileName)) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;
    }
}

char* getMethod() {
    char* line;
    line = malloc(sizeof(char) * 5);
    int i = -1;
    bool done = false;
    while (!done && client.connected()) {
        if (client.available()) {
            i++;
            if (i > 4) {
                done = true;
            }
            line[i] = client.read();
            if (line[i] == ' ') {
                line[i] = '\0';
                done = true;
            }
        }
    }

    return line;
}

void doCmd(int pinNo, char* cmd) {
    if (strcmp(cmd, "stat") == 0) {
        head(200, "text/plain");
        const char* ls = lightStatusHuman(pinNo);
        if (strcmp(ls, "") != 0) client.println(ls);
    }

    if (strcmp(cmd, "on") == 0) {
        updateLights(pinNo, true);
    } else if (strcmp(cmd, "off") == 0) {
        updateLights(pinNo, false);
    }
}

void processCommand(char* pin, char* cmd) {
    if (strcmp(pin, "tv") == 0)
        doCmd(7, cmd);
    else if (strcmp(pin, "bed") == 0)
        doCmd(2, cmd);
    else if (strcmp(pin, "all") == 0) {
        for (int i = 2; i < 8; i++) {
            doCmd(i, cmd);
        }
    }
}

void head(unsigned int status, const char* contentType) {
    if (headWritten) return;
    headWritten = true;
    char header[16];
    sprintf(header, "HTTP/1.1 %d OK", status);
    client.println(header);
    if (contentType != NULL) {
        client.print("Content-Type: ");
        client.println(contentType);
    }
    client.println("Access-control-allow-origin: *");
    client.println("Connection: close");
    client.println();
}

void getResponse() {
    head(200, "text/html");
    pageFile = SD.open(fileName);
    if (pageFile) {
        while (pageFile.available()) {
            client.write(pageFile.read());
        }
        pageFile.close();
    }
}

// We don't care about the header, just skip past this in the stream
void passHeader() {
    unsigned int consecutiveNewlines = 0;
    bool done = false;
    while (!done && client.connected()) {
        if (client.available()) {
            char c = client.read();
            if (c == '\r') continue;
            if (c == '\n')
                consecutiveNewlines++;
            else
                consecutiveNewlines = 0;
            if (consecutiveNewlines == 2) done = true;
        }
    }
}

char* getLine() {
    char* line;
    line = malloc(sizeof(char) * 32);
    int i = -1;
    bool done = false;
    while (!done && client.connected()) {
        if (client.available()) {
            i++;
            if (i > 31) done = true;
            line[i] = client.read();
            if (line[i] == '\0') done = true;
            if (line[i] == '\n' || line[i] == '\r') {
                line[i] = '\0';
                done = true;
            }
        } else
            done = true;
    }
    if (line[i] != '\0') line[i + 1] = '\0';
    return line;
}

void processCmd(char* line) {
    char pin[5];
    char cmd[6];
    int i = 0;
    bool pinDone = false;
    for (; !pinDone && i < sizeof(pin) - 1; i++) {
        if (line[i] == ' ') {
            pin[i] = '\0';
            pinDone = true;
        } else
            pin[i] = line[i];
    }

    for (int j = 0; j < sizeof(cmd) - 1; j++) {
        cmd[j] = line[i + j];
    }

    processCommand(pin, cmd);
}

void readRequest() {
    char* method = getMethod();
    if (strcmp(method, "GET") == 0) {
        getResponse();
        free(method);
        return;
    }
    free(method);
    passHeader();

    bool done = false;
    while (!done && client.connected()) {
        if (client.available()) {
            char* line = getLine();
            processCmd(line);
            free(line);
        } else
            done = true;
    }
}

void loop() {
    headWritten = false;
    client = server.available();
    if (client) {
        readRequest();
        if (!headWritten) {
            head(204, NULL);
        }
        // give the web browser time to receive the data
        delay(1);

        client.stop();
    }
}
