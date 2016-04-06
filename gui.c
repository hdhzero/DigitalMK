#include <gtk/gtk.h>
#include <stdio.h>

/* Polygon stuff */
typedef struct {
    float x;
    float y;
} Point;

GtkWidget* g_drawing_area;

int between(float a, float b, float c) {
    return (c < a && a < b) || (b < a && a < c);
}

int right_of(float a, float b, float c) {
    return b > a || c > a;
}

int is_inside(Point* poly, int n_pts, Point point) {
    int i;
    int cross = 0;
    float y0;
    float y1;
    float x0;
    float x1;
    float x;
    float y;
    float m;

    for (i = 0; i < n_pts - 1; ++i) {
        y0 = poly[i].y;
        y1 = poly[i + 1].y;
        x0 = poly[i].x;
        x1 = poly[i + 1].x;
        x  = point.x;
        y  = point.y;

        if (between(y, y0, y1)) { //&& right_of(x, x0, x1)) {
            /* y - y0 = m(x -  x0)
               (y - y0) / m + x0 = x  with m = (y1 - y0) / (x1 - x0)
               (y - y0) * (x1 - x0) / (y1 - y0) + x0 = x
            */

            m = (y1 - y0) / (x1 - x0);

            if (x > (y - y0) / m + x0) {
                cross++;
            }
        }
    }

    return cross & 1;
}


/* End polygon */

/* Cairo testing stuff */

static cairo_surface_t *surface = NULL;

static void
clear_surface (void)
{
  cairo_t *cr;

  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_destroy (cr);
}

static gboolean
configure_event_cb (GtkWidget         *widget,
                    GdkEventConfigure *event,
                    gpointer           data)
{
  if (surface)
    cairo_surface_destroy (surface);

  surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                               CAIRO_CONTENT_COLOR,
                                               gtk_widget_get_allocated_width (widget),
                                               gtk_widget_get_allocated_height (widget));

  /* Initialize the surface to white */
  clear_surface ();

  /* We've handled the configure event, no need for further processing. */
  return TRUE;
}

static gboolean
draw_cb (GtkWidget *widget,
         cairo_t   *cr,
         gpointer   data)
{
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

int xs[] = {50, 100, 100, 50, 50};
int ys[] = {50, 50, 100, 100, 50};
Point poly1[] = {{50, 50}, {100, 50}, {100, 100}, {50, 100}, {50, 50}};

int n_pts_poly2 = 16;
Point poly2[] = {
    {8.29,0.88}, {6.63,0.72}, {6.45,2.39}, {3.95,0.84}, 
    {1.01,2.62}, {1.35,4.18}, {2.47,4.25}, {2.87,5.70}, 
    {4.33,3.64}, {5.46,6.28}, {6.71,3.97}, {8.25,6.43}, 
    {8.79,4.20}, {10.54,3.15}, {7.44,2.67}, {8.29, 0.88}
};

void draw_poly2(cairo_t* cr) {
    int i;

    if (poly2[0].x < 10) {
        for (i = 0; i < n_pts_poly2 - 1; ++i) {
            poly2[i].x *= 10;
            poly2[i].y *= 10;
        }
    }

    for (i = 0; i < n_pts_poly2 - 1; ++i) {
        cairo_move_to(cr, poly2[i].x, poly2[i].y);
        cairo_line_to(cr, poly2[i + 1].x, poly2[i + 1].y);
    }

    cairo_stroke(cr);
}

void render_path(cairo_t* cr, char* path, double x, double y) {
    FILE* file;
    char cmd;
    double cur_x;
    double cur_y;
    double tx;
    double ty;
    double rad;
    double a1;
    double a2;
    double x1, y1, x2, y2, x3, y3;

    file = fopen(path, "r");
    cur_x = x;
    cur_y = y;

    while (!feof(file)) {
        fscanf(file, "%c", &cmd);

        switch (cmd) {
        case 'm':
            fscanf(file, "%lf %lf", &tx, &ty);
            cur_x += tx;
            cur_y += ty;
            cairo_move_to(cr, cur_x, cur_y);
            break;

        case 'M':
            fscanf(file, "%lf %lf", &tx, &ty);
            cur_x = x + tx;
            cur_y = y + ty;
            cairo_move_to(cr, cur_x, cur_y);
            break;

        case 'l':
            fscanf(file, "%lf %lf", &tx, &ty);
            cur_x += tx;
            cur_y += ty;
            cairo_line_to(cr, cur_x, cur_y);
            break;

        case 'L':
            fscanf(file, "%lf %lf", &tx, &ty);
            cur_x = x + tx;
            cur_y = y + ty;
            cairo_line_to(cr, cur_x, cur_y);
            break;

        case 'a':
            fscanf(file, "%lf %lf %lf %lf %lf", &tx, &ty, &rad, &a1, &a2);
            cur_x += tx;
            cur_y += ty;
            cairo_new_sub_path(cr);
            cairo_arc(cr, cur_x, cur_y, rad, a1, a2);
            break;

        case 'C':
            fscanf(file, "%lf %lf %lf %lf %lf %lf", &x1, &y1, &x2, &y2, &x3, &y3);
            x1 += x;
            y1 += y;
            x2 += x;
            y2 += y;
            x3 += x;
            y3 += y;
            cur_x = x3;
            cur_y = y3;
            cairo_curve_to(cr, x1, y1, x2, y2, x3, y3);
            break;

        case 'z':
            cairo_close_path(cr);
            cur_x = x;
            cur_y = y;
            break;

        default:
            break;
        }
    }

    cairo_stroke(cr);
    fclose(file);
}

void draw_not_gate(cairo_t* cr, gdouble x, gdouble y, gdouble scale) {
    render_path(cr, "gates/not.txt", x, y);
}

void draw_or_gate(cairo_t* cr, gdouble x, gdouble y, gdouble scale) {
    render_path(cr, "gates/or.txt", x, y);
}

void draw_and_gate(cairo_t* cr, gdouble x, gdouble y, gdouble scale) {
    /* vertical line */
    cairo_move_to(cr, x, y);
    cairo_line_to(cr, x, y + 6 * scale);

    /* up horizontal line */
    cairo_move_to(cr, x, y);
    cairo_line_to(cr, x + 4 * scale, y);

    /* down horizontal line */
    cairo_move_to(cr, x, y + 6 * scale);
    cairo_line_to(cr, x + 4 * scale, y + 6 * scale);

    /* up bezier */
    cairo_move_to(cr, x + 4 * scale, y);
    cairo_curve_to(cr, x + 7 * scale, y, x + 7 * scale, y, x + 7 * scale, y + 3 * scale);

    cairo_stroke(cr);
}

/* Draw a rectangle on the surface at the given position */
static void draw_brush (GtkWidget *widget, gdouble x, gdouble y) {
    cairo_t *cr;
    int i;

  /* Paint to the surface, where we store our state */
    cr = cairo_create (surface);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_rectangle(cr, 0, 0, 2000, 1000);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0, 255, 0);
    cairo_set_line_width(cr, 1);

/*    for (i = 0; i < 4; ++i) {
        cairo_move_to(cr, xs[i], ys[i]);
        cairo_line_to(cr, xs[i + 1], ys[i + 1]);
    }

    cairo_stroke(cr);*/
    draw_poly2(cr);
//    draw_and_gate(cr, x, y, 10);
//    draw_not_gate(cr, x, y, 10);
    draw_or_gate(cr, x, y, 10);

    /*cairo_rectangle(cr, 0, 0, 2000, 1000);
    cairo_fill(cr);

    cairo_rectangle (cr, x - 3, y - 3, 6, 6);
    cairo_fill (cr);*/

    cairo_destroy (cr);

    /* Now invalidate the affected region of the drawing area. */
    gtk_widget_queue_draw_area (widget, x - 3, y - 3, 6, 6);
    gtk_widget_queue_draw_area (widget, 0, 0, 2000, 1000);
}

/* End of cairo */

void close_window() {
    gtk_main_quit();
}

GtkWidget* create_main_window(GtkApplication* app) {
    GtkWidget* window;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "VHDL Sim");

    g_signal_connect(window, "destroy", G_CALLBACK(close_window), NULL);
    gtk_container_set_border_width(GTK_CONTAINER(window), 4);

    return window;
}

gboolean motion_notify_event_cb(GtkWidget* widget, GdkEventMotion* event, gpointer data) {
    if (event->state & GDK_BUTTON1_MASK) {

    }

    return TRUE;
}

GtkWidget* glb_drawing_area;
int factor = 1;

gboolean button_press_event_cb(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    int w = gtk_widget_get_allocated_width(glb_drawing_area);
    int h = gtk_widget_get_allocated_height(glb_drawing_area);
    Point p;

    if (event->button == GDK_BUTTON_PRIMARY) {
        printf("%f %f %i %i\n", event->x, event->y, w, h);

        if (surface != NULL) {
            draw_brush (widget, event->x, event->y);
        }

        p.x = event->x;
        p.y = event->y;

        if (is_inside(poly2, n_pts_poly2, p)) {
            printf("inside\n");
            //gtk_widget_set_size_request(glb_drawing_area, 1000 * factor, 1000 * factor);
            factor++;
        } else {
            printf("outside\n");
            //gtk_widget_set_size_request(glb_drawing_area, 1000 / factor, 1000 / factor);
        }
    } else if (event->button == GDK_BUTTON_SECONDARY) {
        printf("%f %f\n", event->x, event->y);
    }

    return TRUE;
}

void show_h_value(GtkAdjustment* adj, gpointer data) {
    printf("value: %f\n", gtk_adjustment_get_value(adj));
}

GtkWidget* create_drawing_area() {
    GtkWidget* box1;
    GtkWidget* box2;
    GtkWidget* h_scroll_bar;
    GtkWidget* v_scroll_bar;
    GtkWidget* drawing_area;
    GtkAdjustment* h;
    GtkAdjustment* v;
    GtkWidget* frame;

    box1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    box2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

    h = gtk_adjustment_new(0.0, 0.0, 100.0, 1.0, 10.0, 0.0);
    v = gtk_adjustment_new(0.0, 0.0, 100.0, 1.0, 10.0, 0.0);
    gtk_adjustment_set_value(h, 50.0);
    gtk_adjustment_set_value(v, 50.0);

    drawing_area = gtk_drawing_area_new();
    glb_drawing_area = drawing_area;

    g_signal_connect(drawing_area, "motion-notify-event",
                     G_CALLBACK(motion_notify_event_cb), NULL);

    g_signal_connect(drawing_area, "button-press-event",
                     G_CALLBACK(button_press_event_cb), NULL);

    g_signal_connect(h, "value-changed", G_CALLBACK(show_h_value), NULL);

    g_signal_connect (drawing_area,"configure-event",
                    G_CALLBACK (configure_event_cb), NULL);

    g_signal_connect (drawing_area, "draw",
                    G_CALLBACK (draw_cb), NULL);

    gtk_widget_set_events(drawing_area, gtk_widget_get_events(drawing_area)
                                      | GDK_BUTTON_PRESS_MASK
                                      | GDK_POINTER_MOTION_MASK);

    h_scroll_bar = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, h);
    v_scroll_bar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, v);

    gtk_box_pack_start(GTK_BOX(box2), drawing_area, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box2), v_scroll_bar, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box1), box2, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box1), h_scroll_bar, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(frame), box1);

    return frame;
}

GtkWidget* create_drawing_area2() {
    GtkWidget* swin;
    GtkScrolledWindow* swin_aux;
    GtkWidget* box2;
    GtkWidget* h_scroll_bar;
    GtkWidget* v_scroll_bar;
    GtkWidget* drawing_area;
    GtkWidget* viewport;
    GtkAdjustment* h;
    GtkAdjustment* v;

    h = gtk_adjustment_new(0.0, 0.0, 100.0, 1.0, 10.0, 0.0);
    v = gtk_adjustment_new(0.0, 0.0, 100.0, 1.0, 10.0, 0.0);

    viewport = gtk_viewport_new(h, v);
    drawing_area = gtk_drawing_area_new();
    g_drawing_area = drawing_area;
    glb_drawing_area = drawing_area;
    gtk_widget_set_size_request(drawing_area, 1000, 1000);

    swin = gtk_scrolled_window_new(h, v);
    swin_aux = (GtkScrolledWindow*) swin;

    gtk_scrolled_window_set_policy(swin_aux, GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_shadow_type(swin_aux, GTK_SHADOW_IN);

    gtk_adjustment_set_value(h, 50.0);
    gtk_adjustment_set_value(v, 50.0);

    g_signal_connect(drawing_area, "motion-notify-event",
                     G_CALLBACK(motion_notify_event_cb), NULL);

    g_signal_connect(drawing_area, "button-press-event",
                     G_CALLBACK(button_press_event_cb), NULL);

    g_signal_connect(h, "value-changed", G_CALLBACK(show_h_value), NULL);

    g_signal_connect (drawing_area,"configure-event",
                    G_CALLBACK (configure_event_cb), NULL);

    g_signal_connect (drawing_area, "draw",
                    G_CALLBACK (draw_cb), NULL);

    gtk_widget_set_events(drawing_area, gtk_widget_get_events(drawing_area)
                                      | GDK_BUTTON_PRESS_MASK
                                      | GDK_POINTER_MOTION_MASK);

    gtk_container_add(GTK_CONTAINER(viewport), drawing_area);
    gtk_container_add(GTK_CONTAINER(swin), viewport);

    return swin;
}

GtkWidget* draw_logical_gates_frame() {
    GtkWidget* gate_frame;
    GtkWidget* arith_frame;
    GtkWidget* gate_toolbar1;
    GtkWidget* gate_toolbar2;
    GtkWidget* arith_toolbar1;
    GtkWidget* box;
    GtkWidget* gate_box;

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gate_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gate_frame = gtk_frame_new("Gates");
    gate_toolbar1 = gtk_toolbar_new();
    gate_toolbar2 = gtk_toolbar_new();

    gtk_toolbar_insert(GTK_TOOLBAR(gate_toolbar1), gtk_tool_button_new(gtk_image_new_from_file("img/xor.png"), "t"), 0);
    gtk_toolbar_insert(GTK_TOOLBAR(gate_toolbar1), gtk_tool_button_new(gtk_image_new_from_file("img/or.png"), "t"), 0);
    gtk_toolbar_insert(GTK_TOOLBAR(gate_toolbar1), gtk_tool_button_new(gtk_image_new_from_file("img/and.png"), "t"), 0);
    gtk_toolbar_insert(GTK_TOOLBAR(gate_toolbar1), gtk_tool_button_new(gtk_image_new_from_file("img/not.png"), "t"), 0);

    gtk_toolbar_insert(GTK_TOOLBAR(gate_toolbar2), gtk_tool_button_new(gtk_image_new_from_file("img/nand.png"), "t"), 0);
    gtk_toolbar_insert(GTK_TOOLBAR(gate_toolbar2), gtk_tool_button_new(gtk_image_new_from_file("img/nor.png"), "t"), 0);
    gtk_toolbar_insert(GTK_TOOLBAR(gate_toolbar2), gtk_tool_button_new(gtk_image_new_from_file("img/xnor.png"), "t"), 0);

    gtk_box_pack_start(GTK_BOX(gate_box), gate_toolbar1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gate_box), gate_toolbar2, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(gate_frame), gate_box);

    arith_frame = gtk_frame_new("Arithmetic");
    arith_toolbar1 = gtk_toolbar_new();
    gtk_toolbar_insert(GTK_TOOLBAR(arith_toolbar1), gtk_tool_button_new(gtk_image_new_from_file("img/nand.png"), "t"), 0);
    gtk_toolbar_insert(GTK_TOOLBAR(arith_toolbar1), gtk_tool_button_new(gtk_image_new_from_file("img/nor.png"), "t"), 0);
    gtk_toolbar_insert(GTK_TOOLBAR(arith_toolbar1), gtk_tool_button_new(gtk_image_new_from_file("img/xnor.png"), "t"), 0);
//    gtk_toolbar_insert(GTK_TOOLBAR(arith_toolbar1), gtk_tool_button_new(gtk_image_new_from_file("and.png"), "t"), 0);
    gtk_container_add(GTK_CONTAINER(arith_frame), arith_toolbar1);


    gtk_box_pack_start(GTK_BOX(box), gate_frame, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), arith_frame, FALSE, FALSE, 10);
//    gtk_box_pack_start(GTK_BOX(box), frame2, FALSE, FALSE, 0);

    return box;
}

GtkWidget* create_main_toolbar() {
    GtkWidget* toolbar;

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
    //gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar2), GTK_ORIENTATION_VERTICAL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_tool_button_new_from_stock(GTK_STOCK_SAVE_AS), 0);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_tool_button_new_from_stock(GTK_STOCK_SAVE), 0);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_tool_button_new_from_stock(GTK_STOCK_OPEN), 0);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_tool_button_new_from_stock(GTK_STOCK_NEW), 0);

    return toolbar;
}

void build_gui(GtkApplication* app, gpointer data) {
    GtkWidget* window;
    GtkWidget* box1;
    GtkWidget* menu;
    GtkWidget* toolbar;
    GtkWidget* toolbar2;
    GtkWidget* main_pane_box;
    GtkWidget* main_pane;
    GtkWidget* info_pane;
    GtkWidget* status_bar;
    GtkWidget* drawing_area;
    GtkWidget* label1;
    GtkWidget* label2;
    int i;

    GtkWidget* button1, *button2;

    button1 = gtk_button_new_with_label("place holder1");
    button2 = gtk_button_new_with_label("place holder2");

    window = create_main_window(app);

    menu = gtk_menu_bar_new();

    
    main_pane = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    info_pane = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    drawing_area = create_drawing_area();

    status_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    label1 = gtk_label_new("current position: (50.0, 70.0)");
    label2 = gtk_label_new("current position: (50.0, 70.0)");
    gtk_box_pack_start(GTK_BOX(status_bar), label1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(status_bar), label2, FALSE, FALSE, 0);

    gtk_paned_pack1(GTK_PANED(main_pane), info_pane, TRUE, TRUE);
    gtk_paned_pack2(GTK_PANED(main_pane), drawing_area, TRUE, TRUE);
    gtk_paned_pack1(GTK_PANED(info_pane), button1, FALSE, FALSE);
    gtk_paned_pack2(GTK_PANED(info_pane), button2, FALSE, FALSE);

    box1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    main_pane_box= gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    gtk_box_pack_start(GTK_BOX(main_pane_box), main_pane, TRUE, TRUE, 0);
//    gtk_box_pack_start(GTK_BOX(main_pane_box), toolbar2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_pane_box), draw_logical_gates_frame(), FALSE, FALSE, 10);

    gtk_box_pack_start(GTK_BOX(box1), menu, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box1), create_main_toolbar(), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box1), main_pane_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box1), status_bar, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), box1);

    gtk_widget_show_all(window);
}

void build_gui2(GtkApplication* app, gpointer data) {
    GtkWidget* window;
    GtkWidget* box1;
    GtkWidget* menu;
    GtkWidget* toolbar;
    GtkWidget* main_pane;
    GtkWidget* info_pane;
    GtkWidget* status_bar;
    GtkWidget* drawing_area;

    GtkWidget* button1, *button2;

    button1 = gtk_button_new_with_label("place holder1");
    button2 = gtk_button_new_with_label("place holder2");

    window = create_main_window(app);

    menu = gtk_menu_bar_new();
    toolbar = gtk_toolbar_new();
    main_pane = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    info_pane = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    status_bar = gtk_statusbar_new();
    drawing_area = create_drawing_area();

    gtk_paned_pack1(GTK_PANED(main_pane), info_pane, TRUE, TRUE);
    gtk_paned_pack2(GTK_PANED(main_pane), drawing_area, TRUE, TRUE);
    gtk_paned_pack1(GTK_PANED(info_pane), button1, FALSE, FALSE);
    gtk_paned_pack2(GTK_PANED(info_pane), button2, FALSE, FALSE);

    box1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    gtk_box_pack_start(GTK_BOX(box1), menu, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box1), toolbar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box1), main_pane, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box1), status_bar, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), box1);

    gtk_widget_show_all(window);
}

void activate(GtkApplication* app, gpointer data) {
    build_gui(app, data);
}

int main(int argc, char* argv[]) {
    GtkApplication* app;
    int status;

    app = gtk_application_new("org.gtk.vhdl.sim", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
