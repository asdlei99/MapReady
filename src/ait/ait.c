#include "ait.h"
#include <stdarg.h>

/* for win32, set the font to the standard windows one */
#if defined(win32)

static char appfontname[128] = "tahoma 8"; /* fallback value */

static void set_app_font (const char *fontname)
{
    GtkSettings *settings;

    if (fontname != NULL && *fontname == 0) return;

    settings = gtk_settings_get_default();

    if (fontname == NULL) {
	g_object_set(G_OBJECT(settings), "gtk-font-name", appfontname, NULL);
    } else {
	GtkWidget *w;
	PangoFontDescription *pfd;
	PangoContext *pc;
	PangoFont *pfont;

	w = gtk_label_new(NULL);
	pfd = pango_font_description_from_string(fontname);
	pc = gtk_widget_get_pango_context(w);
	pfont = pango_context_load_font(pc, pfd);

	if (pfont != NULL) {
	    strcpy(appfontname, fontname);
	    g_object_set(G_OBJECT(settings), "gtk-font-name", appfontname,
			 NULL);
	}

	gtk_widget_destroy(w);
	pango_font_description_free(pfd);
    }
}

char *default_windows_menu_fontspec (void)
{
    gchar *fontspec = NULL;
    NONCLIENTMETRICS ncm;

    memset(&ncm, 0, sizeof ncm);
    ncm.cbSize = sizeof ncm;

    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0)) {
	HDC screen = GetDC(0);
	double y_scale = 72.0 / GetDeviceCaps(screen, LOGPIXELSY);
	int point_size = (int) (ncm.lfMenuFont.lfHeight * y_scale);

	if (point_size < 0) point_size = -point_size;
	fontspec = g_strdup_printf("%s %d", ncm.lfMenuFont.lfFaceName,
				   point_size);
	ReleaseDC(0, screen);
    }

    return fontspec;
}

static void try_to_get_windows_font (void)
{
    gchar *fontspec = default_windows_menu_fontspec();

    if (fontspec != NULL) {
	int match = 0;
	PangoFontDescription *pfd;
	PangoFont *pfont;
	PangoContext *pc;
	GtkWidget *w;

	pfd = pango_font_description_from_string(fontspec);

	w = gtk_label_new(NULL);
	pc = gtk_widget_get_pango_context(w);
	pfont = pango_context_load_font(pc, pfd);
	match = (pfont != NULL);

	pango_font_description_free(pfd);
	g_object_unref(G_OBJECT(pc));
	gtk_widget_destroy(w);

	if (match) set_app_font(fontspec);
	g_free(fontspec);
    }
}

void set_font ()
{
    try_to_get_windows_font();
}

#else /* #if defined(win32) */

/* on unix, GTK will select the appropriate fonts */
void set_font () {}

#endif /* defined(win32) */

/************************************************************************
 * Global variables...
 */

// pointer to the loaded XML file's internal struct
GladeXML *glade_xml;

// list of imagery the user may look at
GtkListStore *images_list;

// We aren't using the asf.h ones because of conflicts in Windows.h
#if defined(win32)
const char PATH_SEPARATOR = ';';
const char DIR_SEPARATOR = '\\';
#else
const char PATH_SEPARATOR = ':';
const char DIR_SEPARATOR = '/';
#endif

static char *
find_in_share(const char * filename)
{
    char * ret = (char *) malloc(sizeof(char) *
                      (strlen(get_asf_share_dir()) + strlen(filename) + 5));
    sprintf(ret, "%s/%s", get_asf_share_dir(), filename);
    return ret;
}

void update_everything()
{
    update_summary();
    geocode_options_changed();
}

void
add_file(char *config_file)
{
    ait_params_t *params = read_settings(config_file);
    if (params) {
        apply_settings_to_gui(params);
        //message_box("Read config file: %s", config_file);

        int do_test = TRUE;
        if (do_test) {
            ait_params_t *p1 = get_settings_from_gui();
            if (!params_diff(params, p1))
                asfPrintWarning("Applied to GUI != Read from file!\n");
            free_ait_params(p1);
        }

    } else {
        message_box("Error reading config file: %s\n", config_file);
    }

    free_ait_params(params);
}

void file_into_textview(char *filename, const char *textview_name)
{
    GtkWidget *text_view = get_widget_checked(textview_name);
    GtkTextBuffer *text_buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    // clear out current contents
    gtk_text_buffer_set_text(text_buffer, "", -1);

    // read the file, populate!
    FILE *fp = fopen(filename, "rt");
    const int max_line_len = 512;

    if (fp)
    {
        // loop through the file, appending each line to the text buffer
        char *buffer = (char *)MALLOC(sizeof(char) * max_line_len);
        while (!feof(fp))
        {
            char *p = fgets(buffer, max_line_len, fp);
            if (p)
            {
                GtkTextIter end;
                gtk_text_buffer_get_end_iter(text_buffer, &end);
                gtk_text_buffer_insert(text_buffer, &end, buffer, -1);
            }
        }

        fclose(fp);
        free(buffer);

        // change to a fixed-width font in the window
        static GtkTextTag *tt = NULL;
        if (!tt)
        {
            tt = gtk_text_buffer_create_tag(text_buffer, "mono",
                                            "font", FW_FNT_NAME, NULL);
        }

        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(text_buffer, &start);
        gtk_text_buffer_get_end_iter(text_buffer, &end);
        gtk_text_buffer_apply_tag(text_buffer, tt, &start, &end);    
    }
    else
    {
        // error opening the file
        char *buf = MALLOC(sizeof(char)*(strlen(filename) + 64));
        sprintf(buf, "Error opening file: %s", filename);
        gtk_text_buffer_set_text(text_buffer, buf, -1);
        free(buf);
    }
}

static void
grab_log(char *config_file)
{
    char *log_file = appendExt(config_file, ".log");
    file_into_textview(log_file, "log_textview");

    GtkWidget *log_label = get_widget_checked("log_file_label");
    gtk_label_set_text(GTK_LABEL(log_label), log_file);
}

char *meta_file_name(const char *data_file_name)
{
    int len = strlen(data_file_name) + 10;
    char *p = findExt(data_file_name);
    if (!p)
    {
        // no extension -- assume a ceos .D file?
        char *ret = MALLOC(sizeof(char)*len);
        strcpy(ret, data_file_name);
        strcat(ret, ".L");
        return ret;
    }

    // move past the "."
    ++p; 

    if (strcmp(p, "D") == 0)
    {
        char *ret = STRDUP(data_file_name);
        ret[strlen(data_file_name) - 1] = 'L';
        return ret;
    }

    if (strcmp(p, "img") == 0)
    {
        char * ret = MALLOC(sizeof(char) * len);
        strcpy(ret, data_file_name);
        ret[strlen(data_file_name) - 3] = '\0';
        strcat(ret, "meta");    
        return ret;
    }

    return STRDUP("");
}

SIGNAL_CALLBACK void
on_ait_main_destroy(GtkWidget *w, gpointer data)
{
    gtk_main_quit();
}

SIGNAL_CALLBACK void
on_execute_button_clicked(GtkWidget *w)
{
    ait_params_t *params = get_settings_from_gui();

    if (strlen(params->name) > 0)
    {
        GtkWidget *execute_button = get_widget_checked("execute_button");
        gtk_widget_set_sensitive(execute_button, FALSE);

        int pid = fork();

        if (pid == 0)
        {
            /* This is the background thread: runs ips */
            write_settings(params);
            ips(params->cfg, params->name, FALSE);
            exit(EXIT_SUCCESS);
        }
        else
        {
            /* This is the gui thread */
            while (waitpid(-1, NULL, WNOHANG) == 0)
            {
                while (gtk_events_pending())
                    gtk_main_iteration();
                
                g_usleep(50);
            }     
        }

        grab_log(params->name);
        gtk_widget_set_sensitive(execute_button, TRUE);
    }
    else
    {
        // must have a config file name to start
        message_box("Error running ips: No configuration file name.");
    }

    free_ait_params(params);
}

SIGNAL_CALLBACK void
on_save_button_clicked(GtkWidget *w)
{
    ait_params_t *params = get_settings_from_gui();
    if (strlen(params->name) > 0) {
        write_settings(params);
        message_box("Wrote configuration file: %s\n", params->name);
    }
    else
        message_box("Please enter a name for the configuration file.");
    free_ait_params(params);
}

SIGNAL_CALLBACK void
on_stop_button_clicked(GtkWidget *w)
{
    const char *tmp_dir = get_asf_tmp_dir();
    char *stop_file = MALLOC(sizeof(char) * (strlen(tmp_dir) + 24));
    sprintf(stop_file, "%s/stop.txt", tmp_dir);
    FILE * fp = fopen(stop_file, "w");
    if (fp) {
        fprintf(fp,
                "Temporary file.\n\n"
                "Flags any asf tools currently running in this directory "
                "to halt processing immediately.\n\n"
                "This file should be deleted once processing has stopped.\n");
        fclose(fp);
    }
    free(stop_file);
}

// Configuration File "Browse"
void config_callback(char *config_file)
{
    GtkWidget *w = get_widget_checked("configuration_file_entry");
    gtk_entry_set_text(GTK_ENTRY(w), config_file);
    add_file(config_file);
    update_summary();
}

SIGNAL_CALLBACK void
on_configuration_file_browse_button_clicked(GtkWidget *w)
{
    browse(FILTER_CFG, config_callback);
}

// Master Image "Browse"
void master_callback(char *master_file)
{
    char *path = MALLOC(sizeof(char)*(strlen(master_file)+2));
    char *data_file = MALLOC(sizeof(char)*(strlen(master_file)+2));
    split_dir_and_file(master_file, path, data_file);
    char *meta_file = meta_file_name(data_file);

    GtkWidget *w = get_widget_checked("master_image_path_entry");
    gtk_entry_set_text(GTK_ENTRY(w), path);

    w = get_widget_checked("master_image_data_entry");
    gtk_entry_set_text(GTK_ENTRY(w), data_file);

    w = get_widget_checked("master_image_metadata_entry");
    gtk_entry_set_text(GTK_ENTRY(w), meta_file);

    free(meta_file);
    free(data_file);
    free(path);

    update_summary();
}

SIGNAL_CALLBACK void
on_master_image_browse_button_clicked(GtkWidget *w)
{
    browse(FILTER_IMAGERY, master_callback);
}

// Slave Image "Browse"
void slave_callback(char *slave_file)
{
    char *path = MALLOC(sizeof(char)*(strlen(slave_file)+2));
    char *data_file = MALLOC(sizeof(char)*(strlen(slave_file)+2));
    split_dir_and_file(slave_file, path, data_file);
    char *meta_file = meta_file_name(data_file);

    GtkWidget *w = get_widget_checked("slave_image_path_entry");
    gtk_entry_set_text(GTK_ENTRY(w), path);

    w = get_widget_checked("slave_image_data_entry");
    gtk_entry_set_text(GTK_ENTRY(w), data_file);

    w = get_widget_checked("slave_image_metadata_entry");
    gtk_entry_set_text(GTK_ENTRY(w), meta_file);

    free(meta_file);
    free(data_file);
    free(path);

    update_summary();
}

SIGNAL_CALLBACK void
on_slave_image_browse_button_clicked(GtkWidget *w)
{
    browse(FILTER_IMAGERY, slave_callback);
}

// Reference DEM "Browse"
void dem_callback(char *dem_file)
{
    GtkWidget *w = get_widget_checked("reference_dem_entry");
    gtk_entry_set_text(GTK_ENTRY(w), dem_file);
    update_summary();
}

SIGNAL_CALLBACK void
on_reference_dem_browse_button_clicked(GtkWidget *w)
{
    browse(FILTER_IMAGERY, dem_callback);
}

void
message_box(const char *format, ...)
{
    char buf[1024];
    int len;

    va_list ap;
    va_start(ap, format);
    len = vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);

    if (len > 1022)
        asfPrintWarning("Lengthy message may have been truncated.\n");

    GtkWidget *dialog, *label;

    dialog = gtk_dialog_new_with_buttons( "Message",
        NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_STOCK_OK,
        GTK_RESPONSE_NONE,
        NULL);

    label = gtk_label_new(buf);

    g_signal_connect_swapped(dialog, 
        "response", 
        G_CALLBACK(gtk_widget_destroy),
        dialog);

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);

    gtk_widget_show_all(dialog);
}

GtkWidget *get_widget_checked(const char *widget_name)
{
    GtkWidget *w = glade_xml_get_widget(glade_xml, widget_name);
    if (!w)
    {
        asfPrintError("get_widget_checked() failed: "
            "The widget %s was not found.\n", widget_name);
    }
    return w;
}

int
main(int argc, char **argv)
{
    gtk_init(&argc, &argv);

    gchar *glade_xml_file = (gchar *) find_in_share("ait.glade");
    //printf("Found ait.glade: %s\n", glade_xml_file);
    glade_xml = glade_xml_new(glade_xml_file, NULL, NULL);

    g_free(glade_xml_file);

    // add version number to window title
    char title[256];
    sprintf(title,
            "AIT - The ASF Interferometry Tool: Version %s", IPS_GUI_VERSION);

    GtkWidget *widget = get_widget_checked("ait_main");
    gtk_window_set_title(GTK_WINDOW(widget), title);

    widget = get_widget_checked("viewed_image_eventbox");
    gtk_widget_set_events(widget, GDK_BUTTON_PRESS_MASK);

    //set_font();
    create_open_dialogs();
    setup_images_treeview();

    if (argc > 1) {
        add_file(argv[1]);
        update_everything();
    }

    glade_xml_signal_autoconnect(glade_xml);
    gtk_main ();

    exit (EXIT_SUCCESS);
}

