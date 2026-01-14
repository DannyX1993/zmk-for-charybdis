
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// ID de la capa donde se activa esta función (Debe coincidir con tu keymap)
#define NAV_LAYER 1

// Sensibilidad (Ajustar al gusto)
#define MOVE_THRESHOLD 15 // Para movimiento normal
#define FLICK_THRESHOLD 60 // Para detección de movimiento rápido

static int16_t x_accum = 0;
static int16_t y_accum = 0;

// Helper para enviar teclas simples
void send_key(uint8_t key) {
    zmk_hid_keyboard_press(key);
    zmk_hid_keyboard_release(key);
}

// Helper para enviar Ctrl + Tecla (Saltar palabras)
void send_ctrl_key(uint8_t key) {
    zmk_hid_keyboard_press(HID_USAGE_KEY_KEYBOARD_LEFTCONTROL);
    zmk_hid_keyboard_press(key);
    zmk_hid_keyboard_release(key);
    zmk_hid_keyboard_release(HID_USAGE_KEY_KEYBOARD_LEFTCONTROL);
}

int text_nav_listener(const zmk_event_t *eh) {
    const struct zmk_input_event *val = as_zmk_input_event(eh);

    // 1. Filtramos eventos que no sean de movimiento relativo
    if(!val || val->type != INPUT_EV_REL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // 2. Solo activamos is estamos en la capa correcta
    if(!zmk_keymap_layer_active(NAV_LAYER)) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // --- EJE X: Caracteres y Palabras ---
    if(val->code == INPUT_REL_X) {
        // Detección de "Flick" (Movimiento rápido) -> Saltar Palabra
        if(val->value > FLICK_THRESHOLD) {
            send_ctrl_key(HID_USAGE_KEY_KEYBOARD_RIGHTARROW); // Palabra Derecha
            x_accum = 0;
            return ZMK_EV_EVENT_HANDLED;
        } else if(val->value < -FLICK_THRESHOLD) {
            send_ctrl_key(HID_USAGE_KEY_KEYBOARD_LEFTARROW); // Palabra Izquierda
            x_accum = 0;
            return ZMK_EV_EVENT_HANDLED;
        }

        // Movimiento Normal -> Carácter a Carácter
        x_accum += val->value;
        if(x_accum > MOVE_THRESHOLD) {
            send_key(HID_USAGE_KEY_KEYBOARD_RIGHTARROW);
            x_accum = 0;
        } else if(x_accum < -MOVE_THRESHOLD) {
            send_key(HID_USAGE_KEY_KEYBOARD_LEFTARROW);
            x_accum = 0;
        }
        return ZMK_EV_EVENT_HANDLED;
    }

    // --- EJE Y: Líneas y Bloques ---
    if(val->code == INPUT_REL_Y) {
        // Detección de "Flick" -> Saltar 5 líneas (simulando Page Down suave)
        if(val->value > FLICK_THRESHOLD) {
            // Enviamos 5 veces la fecha hacia abajo
            for(int i = 0; i < 5; i++) send_key(HID_USAGE_KEY_KEYBOARD_DOWNARROW);
            y_accum = 0;
            return ZMK_EV_EVENT_HANDLED;
        } else if(val->value < -FLICK_THRESHOLD) {
            // Enviamos 5 veces la flecha arriba
            for(int i = 0; i < 5; i++) send_key(HID_USAGE_KEY_KEYBOARD_UPARROW);
            y_accum = 0;
            return ZMK_EV_EVENT_HANDLED;
        }

        // Movimiento Normal -> Línea a Línea
        y_accum += val->value;
        if(y_accum > MOVE_THRESHOLD) {
            send_key(HID_USAGE_KEY_KEYBOARD_DOWNARROW);
            y_accum = 0;
        } else if(y_accum < -MOVE_THRESHOLD) {
            send_key(HID_USAGE_KEY_KEYBOARD_UPARROW);
            y_accum = 0;
        }

        return ZMK_EV_EVENT_HANDLED
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(text_nav, text_nav_listener);
ZMK_SUBSCRIPTION(text_nav, zmk_input_event);