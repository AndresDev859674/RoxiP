#ifndef ROXIP_UI_H
#define ROXIP_UI_H

#include "kernel.h"

/* UI Core & Terminal Prototypes */
void term_add_line(const char* text);
void term_clear_history(void);
void draw_top_header(void);
void redraw_current_view(void);

/* CLI Module */
void render_cli(void);

/* Desktop Module */
void render_desktop(void);
void render_perf_window(void);

/* Applications Module (ztop, vfm, micro) */
void render_ztop_frame(void);
void render_vfm(int selected_index);
void render_micro_editor(void);
void open_micro_editor(const char* filename);

/* Demos Module (floatcube, rgbdemo) */
void render_floatcube_frame(void);
void render_rgbdemo(void);

#endif