#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h> // Header nativo de Zephyr Input
#include <zephyr/logging/log.h>
#include <zmk/keymap.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// ID de la capa donde se activa esta función (Debe coincidir con tu keymap)
#define NAV_LAYER 1

// Sensibilidad (Ajustar al gusto)
#define MOVE_THRESHOLD 15 // Para movimiento normal
#define FLICK_THRESHOLD 60 // Para detección de movimiento rápido

// CÓDIGOS HID (Definidos manualmente para evitar errores de headers faltantes)
// Keyboard Page (0x07)
#define H_RIGHT 0x4F
#define H_LEFT  0x50
#define H_DOWN  0x51
#define H_UP    0x52
#define H_LCTRL 0xE0

static int16_t x_accum = 0;
static int16_t y_accum = 0;

// Helper para enviar teclas simples
void tap_key(uint8_t key) {
    zmk_hid_keyboard_press(key);
    zmk_hid_keyboard_release(key);
}

// Helper para enviar Ctrl + Tecla (Saltar palabras)
void tap_ctrl_key(uint8_t key) {
    zmk_hid_keyboard_press(H_LCTRL);
    zmk_hid_keyboard_press(key);
    zmk_hid_keyboard_release(key);
    zmk_hid_keyboard_release(H_LCTRL);
}

static void text_nav_callback(struct input_event *evt) {
    // 1. Filtrar solo eventos de movimiento relativo (REL)
    if (evt->type != INPUT_EV_REL) {
        return;
    }

    // 2. Verificar si la capa está activa
    if (!zmk_keymap_layer_active(NAV_LAYER)) {
        return;
    }

    // --- EJE X (Horizontal) ---
    if (evt->code == INPUT_REL_X) {
        // Flick (Rápido) -> Palabra
        if (evt->value > FLICK_THRESHOLD) {
            tap_ctrl_key(H_RIGHT);
            x_accum = 0;
        } else if (evt->value < -FLICK_THRESHOLD) {
            tap_ctrl_key(H_LEFT);
            x_accum = 0;
        } 
        // Normal (Lento) -> Carácter
        else {
            x_accum += evt->value;
            if (x_accum > MOVE_THRESHOLD) {
                tap_key(H_RIGHT);
                x_accum = 0;
            } else if (x_accum < -MOVE_THRESHOLD) {
                tap_key(H_LEFT);
                x_accum = 0;
            }
        }
    }

    // --- EJE Y (Vertical) ---
    if (evt->code == INPUT_REL_Y) {
        // Flick (Rápido) -> 5 Líneas
        if (evt->value > FLICK_THRESHOLD) {
            for(int i=0; i<5; i++) tap_key(H_DOWN);
            y_accum = 0;
        } else if (evt->value < -FLICK_THRESHOLD) {
            for(int i=0; i<5; i++) tap_key(H_UP);
            y_accum = 0;
        } 
        // Normal (Lento) -> 1 Línea
        else {
            y_accum += evt->value;
            if (y_accum > MOVE_THRESHOLD) {
                tap_key(H_DOWN);
                y_accum = 0;
            } else if (y_accum < -MOVE_THRESHOLD) {
                tap_key(H_UP);
                y_accum = 0;
            }
        }
    }
}

// REGISTRO DEL CALLBACK (Zephyr 3.x / ZMK v0.3)
// El primer argumento es NULL para escuchar a todos los dispositivos (el trackball)
INPUT_CALLBACK_DEFINE(NULL, text_nav_callback);