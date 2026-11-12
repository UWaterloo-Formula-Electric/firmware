#include <stdio.h>
#include <string.h>
#include <math.h>
#include <raylib.h>
#include "../inc/render.h"

#define GRID_COLS 10
#define NUM_FIELDS 8

static const Color COLOUR_BG = {14, 14, 14, 255};
static const Color COLOUR_PANEL = {34, 34, 34, 255};
static const Color COLOUR_BORDER = {110, 110, 110, 255};
static const Color COLOUR_LABEL = {190, 190, 190, 255};
static const Color COLOUR_INVALID = {40, 40, 40, 255};
static const Color COLOUR_INVALID_TEXT = {150, 150, 150, 255};
static const Color COLOUR_STALE = {85, 85, 85, 255};
static const Color COLOUR_OK = {0, 110, 45, 255};
static const Color COLOUR_WARN = {255, 176, 0, 255};
static const Color COLOUR_CRIT = {210, 0, 0, 255};

// names from bmu/Inc/controlStateMachine.h, the DBC value table stops at 7
static const char *bms_state_names[] = {
    [SELF_CHECK] = "SELF CHECK",
    [WAIT_SYSTEM_UP] = "WAIT SYSTEM UP",
    [HV_DISABLE] = "HV DISABLE",
    [HV_ENABLE] = "HV ENABLE",
    [PRECHARGE] = "PRECHARGE",
    [DISCHARGE] = "DISCHARGE",
    [CHARGING] = "CHARGING",
    [FAILURE_FATAL] = "FAILURE FATAL",
    [FAILURE_CBRB_DISABLED] = "FAILURE CBRB DISABLED",
    [FAILURE_CBRB_ENABLED] = "FAILURE CBRB ENABLED",
    [BALANCING] = "BALANCING"
};

typedef struct {
    char label[32];
    char value[32];
    status_t status;
    float weight;
    int highlight;
} field_t;

static Vector2 measure (const char *text, float size) {
    return MeasureTextEx(GetFontDefault(), text, size, size / 10.0f);
}

static void draw_text (const char *text, float x, float y, float size, Color colour) {
    DrawTextEx(GetFontDefault(), text, (Vector2){x, y}, size, size / 10.0f, colour);
}

static float fit_size (const char *text, float max_size, float max_width) {
    float w = measure(text, max_size).x;
    return w > max_width ? max_size * max_width / w : max_size;
}

static void draw_text_centred (const char *text, Rectangle r, float max_size, Color colour) {
    float size = fit_size(text, max_size, r.width);
    Vector2 m = measure(text, size);
    draw_text(text, r.x + (r.width - m.x) / 2, r.y + (r.height - m.y) / 2, size, colour);
}

// default font has no em-dash glyph
static void draw_dash (float cx, float cy, float size, Color colour) {
    DrawRectangleRec((Rectangle){cx - size / 2, cy - size * 0.05f, size, size * 0.1f}, colour);
}

static Rectangle inset (Rectangle r, float d) {
    return (Rectangle){r.x + d, r.y + d, r.width - 2 * d, r.height - 2 * d};
}

static Color status_bg (status_t s) {
    switch (s) {
        case STATUS_OK: return COLOUR_OK;
        case STATUS_LOW:
        case STATUS_HIGH: return COLOUR_WARN;
        case STATUS_LOW_CRITICAL:
        case STATUS_HIGH_CRITICAL: return COLOUR_CRIT;
        case STATUS_STALE: return COLOUR_STALE;
        default: return COLOUR_INVALID;
    }
}

static Color status_fg (status_t s) {
    switch (s) {
        case STATUS_LOW:
        case STATUS_HIGH: return BLACK;
        case STATUS_STALE: return Fade(WHITE, 0.45f);
        case STATUS_INVALID: return COLOUR_INVALID_TEXT;
        default: return WHITE;
    }
}

static const char *status_name (status_t s) {
    switch (s) {
        case STATUS_OK: return "OK";
        case STATUS_LOW: return "LOW";
        case STATUS_HIGH: return "HIGH";
        case STATUS_LOW_CRITICAL: return "LOW CRIT";
        case STATUS_HIGH_CRITICAL: return "HIGH CRIT";
        case STATUS_STALE: return "STALE";
        default: return "NO DATA";
    }
}

static void append (char *buf, size_t len, const char *text) {
    size_t used = strlen(buf);
    snprintf(buf + used, len - used, "%s", text);
}

static void draw_field (const field_t *f, Rectangle r) {
    int alert = f->status != STATUS_OK && f->status != STATUS_STALE && f->status != STATUS_INVALID;
    Color bg = f->highlight ? WHITE : alert ? status_bg(f->status) : COLOUR_PANEL;
    Color fg = f->highlight ? BLACK : f->status == STATUS_OK ? WHITE : status_fg(f->status);
    Color label = f->highlight || alert ? fg : COLOUR_LABEL;
    float pad = r.height * 0.08f;
    float label_size = r.height * 0.22f;
    DrawRectangleRec(r, bg);
    draw_text(f->label, r.x + pad, r.y + pad, fit_size(f->label, label_size, r.width - 2 * pad), label);
    Rectangle value = {r.x + pad, r.y + pad + label_size, r.width - 2 * pad, r.height - 2 * pad - label_size};
    if (f->status == STATUS_INVALID) {
        draw_dash(value.x + value.width / 2, value.y + value.height / 2, value.height * 0.6f, fg);
    } else {
        draw_text_centred(f->value, value, value.height * 0.9f, fg);
    }
}

static void draw_summary (const state_t *state, const evaluation_t *eval, Rectangle band, float gap) {
    field_t f[NUM_FIELDS] = {0};
    status_t vmin = eval->valid_cells ? eval->cellStatus[eval->v_min_idx] : STATUS_INVALID;
    status_t vmax = eval->valid_cells ? eval->cellStatus[eval->v_max_idx] : STATUS_INVALID;
    status_t tmax = eval->valid_temps ? eval->tempStatus[eval->t_max_idx] : STATUS_INVALID;
    int mode = state->StateBMS == CHARGING || state->StateBMS == BALANCING;

    snprintf(f[0].label, sizeof f[0].label, "PACK");
    snprintf(f[0].value, sizeof f[0].value, "%.1f V", state->AMS_PackVoltage);
    f[0].status = eval->AMSStatus;
    snprintf(f[1].label, sizeof f[1].label, "CURRENT");
    snprintf(f[1].value, sizeof f[1].value, "%.1f A", state->currentBusHV);
    f[1].status = eval->busStatus;
    snprintf(f[2].label, sizeof f[2].label, vmin == STATUS_INVALID ? "MIN CELL" : "MIN CELL #%d", eval->v_min_idx);
    snprintf(f[2].value, sizeof f[2].value, "%.3f V", eval->v_min);
    f[2].status = vmin;
    snprintf(f[3].label, sizeof f[3].label, vmax == STATUS_INVALID ? "MAX CELL" : "MAX CELL #%d", eval->v_max_idx);
    snprintf(f[3].value, sizeof f[3].value, "%.3f V", eval->v_max);
    f[3].status = vmax;
    snprintf(f[4].label, sizeof f[4].label, tmax == STATUS_INVALID ? "HOTTEST" : "HOTTEST #%d", eval->t_max_idx);
    snprintf(f[4].value, sizeof f[4].value, "%.1f C", eval->t_max);
    f[4].status = tmax;
    snprintf(f[5].label, sizeof f[5].label, "SOC");
    snprintf(f[5].value, sizeof f[5].value, "%.1f %%", state->StateBatteryChargeHV);
    f[5].status = eval->batteryStatus;
    snprintf(f[6].label, sizeof f[6].label, "BMS STATE");
    if ((unsigned)state->StateBMS < sizeof bms_state_names / sizeof bms_state_names[0]) {
        snprintf(f[6].value, sizeof f[6].value, "%s", bms_state_names[state->StateBMS]);
    } else {
        snprintf(f[6].value, sizeof f[6].value, "UNKNOWN (%d)", (int)state->StateBMS);
    }
    f[6].status = eval->batteryStatus;
    snprintf(f[7].label, sizeof f[7].label, "CHG / BAL");
    snprintf(f[7].value, sizeof f[7].value, "%s", mode ? bms_state_names[state->StateBMS] : "INACTIVE");
    f[7].status = eval->batteryStatus;
    f[7].highlight = mode && eval->batteryStatus == STATUS_OK;

    float weights[NUM_FIELDS] = {1.0f, 1.0f, 1.1f, 1.1f, 1.0f, 0.8f, 1.8f, 1.2f};
    float total = 0;
    for (int i = 0; i < NUM_FIELDS; i++) total += weights[i];
    float usable = band.width - gap * (NUM_FIELDS + 1);
    float x = band.x + gap;
    for (int i = 0; i < NUM_FIELDS; i++) {
        float w = usable * weights[i] / total;
        draw_field(&f[i], (Rectangle){x, band.y + gap, w, band.height - gap});
        x += w + gap;
    }
}

static void draw_grid (const state_t *state, const evaluation_t *eval, Rectangle band, float gap) {
    int rows = (NUM_VOLTAGE_CELLS + GRID_COLS - 1) / GRID_COLS;
    float tw = (band.width - gap * (GRID_COLS + 1)) / GRID_COLS;
    float th = (band.height - gap * (rows + 1)) / rows;
    float index_size = fit_size("000", th * 0.32f, tw * 0.18f);
    float value_size = fit_size("0.000", th * 0.7f, tw * 0.7f);
    char buf[16];
    for (int i = 0; i < NUM_VOLTAGE_CELLS; i++) {
        status_t s = eval->cellStatus[i];
        Rectangle tile = {
            band.x + gap + (i % GRID_COLS) * (tw + gap),
            band.y + gap + (i / GRID_COLS) * (th + gap),
            tw, th
        };
        Rectangle value = {tile.x + tw * 0.2f, tile.y, tw * 0.8f, th};
        DrawRectangleRec(tile, status_bg(s));
        snprintf(buf, sizeof buf, "%d", i);
        draw_text(buf, tile.x + th * 0.1f, tile.y + th * 0.1f, index_size, status_fg(s));
        if (s == STATUS_INVALID) {
            draw_dash(value.x + value.width / 2, value.y + value.height / 2, value_size * 1.5f, status_fg(s));
        } else {
            snprintf(buf, sizeof buf, "%.3f", state->voltage[i]);
            draw_text_centred(buf, value, value_size, status_fg(s));
        }
    }
}

static void draw_control (Rectangle band, float gap) {
    static const char *buttons[] = {"START CHARGE", "STOP CHARGE", "START BALANCE"};
    Rectangle r = inset(band, gap);
    float line = fmaxf(1.0f, gap * 0.35f);
    float label_size = r.height * 0.18f;
    float x = r.x + gap * 2, y = r.y + gap;
    DrawRectangleLinesEx(r, line, COLOUR_BORDER);

    draw_text("Control", x, y, label_size, COLOUR_LABEL);
    x += measure("Control", label_size).x + label_size * 0.3f;
    draw_dash(x + label_size * 0.5f, y + label_size * 0.5f, label_size, COLOUR_LABEL);
    x += label_size * 1.3f;
    draw_text("not yet enabled", x, y, label_size, COLOUR_LABEL);

    float top = y + label_size + gap;
    Rectangle row = {r.x + gap * 2, top, r.width - gap * 4, r.y + r.height - gap * 2 - top};
    float bw = (row.width - gap * 4) / 3;
    for (int i = 0; i < 3; i++) {
        Rectangle b = {row.x + i * (bw + gap * 2), row.y, bw, row.height};
        DrawRectangleLinesEx(b, line, COLOUR_STALE);
        draw_text_centred(buttons[i], inset(b, gap), b.height * 0.35f, COLOUR_STALE);
    }
}

static void draw_chip (const char *text, status_t s, Rectangle r) {
    DrawRectangleRec(r, status_bg(s));
    draw_text_centred(text, inset(r, r.height * 0.1f), r.height * 0.7f, s == STATUS_STALE ? WHITE : status_fg(s));
}

void render (const state_t *state, const evaluation_t *eval) {
    float w = GetScreenWidth(), h = GetScreenHeight();
    float gap = fminf(w, h) * 0.006f;
    Rectangle summary = {0, 0, w, h * 0.12f};
    Rectangle control = {0, h * 0.75f, w, h * 0.15f};
    Rectangle strip = {0, h * 0.90f, w, h * 0.10f};
    Rectangle grid = {0, summary.height, w, control.y - summary.height};

    int cells_stale = 0, cells_critical = 0, temps_stale = 0;
    for (int i = 0; i < NUM_VOLTAGE_CELLS; i++) {
        status_t s = eval->cellStatus[i];
        if (s == STATUS_STALE) cells_stale = 1;
        if (s == STATUS_LOW_CRITICAL || s == STATUS_HIGH_CRITICAL) cells_critical = 1;
    }
    for (int i = 0; i < NUM_TEMP_CELLS; i++) {
        if (eval->tempStatus[i] == STATUS_STALE) temps_stale = 1;
    }
    status_t cells_comms = cells_stale ? STATUS_STALE : eval->valid_cells ? STATUS_OK : STATUS_INVALID;
    status_t temps_comms = temps_stale ? STATUS_STALE : eval->valid_temps ? STATUS_OK : STATUS_INVALID;

    ClearBackground(COLOUR_BG);
    draw_summary(state, eval, summary, gap);
    draw_grid(state, eval, grid, gap);
    draw_control(control, gap);

    char text[64];
    Rectangle body = {strip.x + gap, strip.y, strip.width - 2 * gap, strip.height - gap};
    Rectangle chips = {body.x, body.y, body.width, body.height * 0.4f};
    Rectangle banner = {body.x, chips.y + chips.height + gap, body.width, body.height * 0.6f - gap};
    float cw = (chips.width - gap * 4) / 5;
    snprintf(text, sizeof text, "CELLS %d/%d %s", eval->valid_cells, NUM_VOLTAGE_CELLS, status_name(cells_comms));
    draw_chip(text, cells_comms, (Rectangle){chips.x, chips.y, cw, chips.height});
    snprintf(text, sizeof text, "TEMPS %d/%d %s", eval->valid_temps, NUM_TEMP_CELLS, status_name(temps_comms));
    draw_chip(text, temps_comms, (Rectangle){chips.x + (cw + gap), chips.y, cw, chips.height});
    snprintf(text, sizeof text, "BUS %s", status_name(eval->busStatus));
    draw_chip(text, eval->busStatus, (Rectangle){chips.x + 2 * (cw + gap), chips.y, cw, chips.height});
    snprintf(text, sizeof text, "AMS %s", status_name(eval->AMSStatus));
    draw_chip(text, eval->AMSStatus, (Rectangle){chips.x + 3 * (cw + gap), chips.y, cw, chips.height});
    snprintf(text, sizeof text, "BMU STATUS %s", status_name(eval->batteryStatus));
    draw_chip(text, eval->batteryStatus, (Rectangle){chips.x + 4 * (cw + gap), chips.y, cw, chips.height});

    int any_stale = cells_stale || temps_stale || eval->busStatus == STATUS_STALE
        || eval->AMSStatus == STATUS_STALE || eval->batteryStatus == STATUS_STALE;
    if (!cells_critical && !any_stale) {
        DrawRectangleRec(banner, COLOUR_PANEL);
        return;
    }

    char msg[128] = "";
    if (cells_critical) append(msg, sizeof msg, "CRITICAL CELL VOLTAGE");
    if (cells_critical && any_stale) append(msg, sizeof msg, "   |   ");
    if (any_stale) {
        append(msg, sizeof msg, "STALE:");
        if (cells_stale) append(msg, sizeof msg, " CELLS");
        if (temps_stale) append(msg, sizeof msg, " TEMPS");
        if (eval->busStatus == STATUS_STALE) append(msg, sizeof msg, " BUS");
        if (eval->AMSStatus == STATUS_STALE) append(msg, sizeof msg, " AMS");
        if (eval->batteryStatus == STATUS_STALE) append(msg, sizeof msg, " BMU");
    }

    Color alarm = cells_critical ? COLOUR_CRIT : COLOUR_WARN;
    Color ink = cells_critical ? WHITE : BLACK;
    if (fmod(GetTime(), 1.0) >= 0.5) {
        Color t = alarm;
        alarm = ink;
        ink = t;
    }
    DrawRectangleRec(banner, alarm);
    draw_text_centred(msg, inset(banner, gap), banner.height * 0.8f, ink);
    DrawRectangleLinesEx((Rectangle){0, 0, w, h}, fminf(w, h) * 0.012f, alarm);
}
