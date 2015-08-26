#include <pebble.h>

#define KEY_LOCATION 0
#define KEY_TEMPERATURE 1

static Window *window;
static TextLayer *timeText;
static TextLayer *location;
static TextLayer *temperature;
static TextLayer *flippinText;
static TextLayer *statusText;

static void updateTime() {
    time_t currentTime = time(NULL);
    struct tm *tick = localtime(&currentTime);

    static char timeBuffer[] = "00:00";

    if (clock_is_24h_style() == true) {
        strftime(timeBuffer, sizeof("00:00"), "%H:%M", tick);
    } else {
        strftime(timeBuffer, sizeof("00:00"), "%I:%M", tick);
    }

    text_layer_set_text(timeText, timeBuffer);
}

static void loadWindow(Window *window) {
    timeText = text_layer_create(GRect(0, 0, 144, 50));
    text_layer_set_background_color(timeText, GColorWhite);
    text_layer_set_text_color(timeText, GColorBlack);
    text_layer_set_font(timeText, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(timeText, GTextAlignmentCenter);

    location = text_layer_create(GRect(0, 52, 144, 24));
    text_layer_set_background_color(location, GColorBlack);
    text_layer_set_text_color(location, GColorWhite);
    text_layer_set_font(location, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(location, GTextAlignmentCenter);
    text_layer_set_text(location, "Fucking Nowhere");

    temperature = text_layer_create(GRect(0, 73, 144, 38));
    text_layer_set_background_color(temperature, GColorBlack);
    text_layer_set_text_color(temperature, GColorWhite);
    text_layer_set_font(temperature, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    text_layer_set_text_alignment(temperature, GTextAlignmentCenter);
    text_layer_set_text(temperature, "--\u00B0C");

    flippinText = text_layer_create(GRect(0, 104, 144, 35));
    text_layer_set_background_color(flippinText, GColorBlack);
    text_layer_set_text_color(flippinText, GColorWhite);
    text_layer_set_font(flippinText, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(flippinText, GTextAlignmentCenter);
    text_layer_set_text(flippinText, "IT'S FUCKING");

    statusText = text_layer_create(GRect(0, 133, 144, 35));
    text_layer_set_background_color(statusText, GColorBlack);
    text_layer_set_text_color(statusText, GColorWhite);
    text_layer_set_font(statusText, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(statusText, GTextAlignmentCenter);
    text_layer_set_text(statusText, "LOADING!");

    layer_add_child(window_get_root_layer(window), text_layer_get_layer(timeText));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(location));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(temperature));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(flippinText));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(statusText));
}

static void unloadWindow(Window *window) {
    text_layer_destroy(timeText);
    text_layer_destroy(location);
    text_layer_destroy(temperature);
    text_layer_destroy(flippinText);
    text_layer_destroy(statusText);
}

static void tickHandler(struct tm *tick, TimeUnits units) {
    updateTime();

    if (tick->tm_min % 15 == 0) {
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        dict_write_uint8(iter, 0, 0);
        app_message_outbox_send();
    }
}

static void receivedCallback(DictionaryIterator *iterator, void *context) {
    static char locationBuffer[32];
    static char temperatureBuffer[8];
    static char statusBuffer[10];
    static int temperatureValue;
    int weatherReceived = 0;

    Tuple *tuple = dict_read_first(iterator);

    while (tuple != NULL) {
        weatherReceived = 1;

        switch (tuple->key) {
            case KEY_LOCATION:
                snprintf(locationBuffer, sizeof(locationBuffer), "Fucking %s",
                    tuple->value->cstring);
                break;

            case KEY_TEMPERATURE:
                temperatureValue = (int)tuple->value->int32;
                snprintf(temperatureBuffer, sizeof(temperatureBuffer), "%d\u00B0C",
                    temperatureValue);

                if (temperatureValue <= -12) {
                    strcpy(statusBuffer, "GLACIAL!");
                } else if (temperatureValue <= -5) {
                    strcpy(statusBuffer, "ARCTIC!");
                } else if (temperatureValue <= 0) {
                    strcpy(statusBuffer, "FREEZING!");
                } else if (temperatureValue <= 9) {
                    strcpy(statusBuffer, "COLD!");
                } else if (temperatureValue <= 15) {
                    strcpy(statusBuffer, "CHILL!");
                } else if (temperatureValue <= 21) {
                    strcpy(statusBuffer, "OKAY!");
                } else if (temperatureValue <= 26) {
                    strcpy(statusBuffer, "WARM!");
                } else if (temperatureValue <= 32) {
                    strcpy(statusBuffer, "HOT!");
                } else {
                    strcpy(statusBuffer, "ROASTING!");
                }

                break;

            default:
                break;
        }

        tuple = dict_read_next(iterator);
    }

    if (!weatherReceived) {
        strcpy(locationBuffer, "Fucking Nowhere");
        strcpy(temperatureBuffer, "\u00B0C");
        strcpy(statusBuffer, "UNKNOWN!");
    }

    text_layer_set_text(location, locationBuffer);
    text_layer_set_text(temperature, temperatureBuffer);
    text_layer_set_text(statusText, statusBuffer);
}

static void droppedCallback(AppMessageResult reason, void *context) {
}

static void failedCallback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
}

static void sentCallback(DictionaryIterator *iterator, void *context) {
}

static void init() {
    window = window_create();

    window_set_window_handlers(window, (WindowHandlers) {
        .load = loadWindow,
        .unload = unloadWindow
    });

    window_stack_push(window, true);

    updateTime();

    tick_timer_service_subscribe(MINUTE_UNIT, tickHandler);

    app_message_register_inbox_received(receivedCallback);
    app_message_register_inbox_dropped(droppedCallback);
    app_message_register_outbox_failed(failedCallback);
    app_message_register_outbox_sent(sentCallback);

    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
