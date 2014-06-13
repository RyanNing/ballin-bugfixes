#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "RemoteFileManager.h"

/*void file_walk()
{
    reset_file_data();
    char dir[MAX_PATH];
    WIN32_FIND_DATA FindData;
    GetCurrentDirectory(chDIMOF(dir), dir);
    SetCurrentDirectory(dir);
    HANDLE hFind = FindFirstFile(TEXT("*.*"), &FindData);
    files[file_count++] = FindData;
    BOOL fOk = (hFind!=INVALID_HANDLE_VALUE);
    while(fOk)
    {
        fOk = FindNextFile(hFind, &FindData);
        if(fOk)
        {
            files[file_count++] = FindData;
        }
    }
}*/

void action()
{
    // printf("drawing file list\n");
    gtk_list_store_clear(GTK_LIST_STORE(store));
    GtkTreeIter iter;
    int i;
    for(i=0;i<file_count;i++)
    {
        long int fileSize = atoll(files[i].file_size);//files[i].nFileSizeHigh * 0x100000000 + files[i].nFileSizeLow;
        long int filesize = fileSize;
        char file_size[50] = {0};
        int j;
        char unit[4][20] = { "Byte", "KB", "MB", "GB" };
        for(j=0;j<3;j++)
        {
            if(fileSize/1024>0)
            {
                fileSize /= 1024;
                if(fileSize/1024<0||j==2)
                {
                    sprintf(file_size, "%6ld ", fileSize);
                    strcat(file_size, unit[j+1]);
                    break;
                }
            }
            else
            {
                sprintf(file_size, "%6ld ", fileSize);
                strcat(file_size, unit[j]);
                break;
            }
        }
        // time_t time;
        // FileTimeToTime_t(files[i].ftLastWriteTime, &time);
        // BOOL fIsDir = files[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        // if(fIsDir&&IsChildDir(&files[i]))
        if(fileSize==-1)
        {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store,
                &iter,
                ICON,       gdk_pixbuf_new_from_file("./resources/folder.png", NULL),
                FILENAME,   _U(files[i].cFileName),
                LASTUPDATE, files[i].time,
                TYPE,       1,
                FILEBYTES,  0,
                -1);
        }
        else
        {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store,
                &iter,
                ICON,       icon_type(_U(files[i].cFileName)),
                FILENAME,   _U(files[i].cFileName),
                FILESIZE,   file_size,
                LASTUPDATE, files[i].time,
                TYPE,       0,
                FILEBYTES,  filesize,
                -1);
        }
    }
}

void destroy_bar(GtkWidget *widget, gpointer data)
{
    ProgressData * pdata = (ProgressData *)data;
    // gtk_widget_set_sensitive(pdata->parent, TRUE);
    // gtk_window_stick(GTK_WINDOW(pdata->window));
    gtk_widget_destroy(pdata->window);
    gtk_timeout_remove(pdata->timer);
    pdata->timer = 0;
    pdata->window = NULL;
    g_free(pdata);
}

// void uploading(gpointer data, gint source, GdkInputCondition condition)
gint uploading(gpointer data)
{
    // if(condition==GDK_INPUT_WRITE)
    {
        ProgressData * pdata = (ProgressData *)data;
        // gtk_window_stick(GTK_WINDOW(pdata->window));
        gdouble new_val;
        int bytes_read = 0;
        memset(buffer, 0, sizeof(buffer));
        DWORD temp;
        ReadFile(pdata->hFile, buffer, BUFFER_SIZE, &temp, NULL);
        bytes_read = send(s, buffer, BUFFER_SIZE, 0);
        if(bytes_read<0)
        {
            WSACleanup();
            while(bytes_read<0)
            {
                Sleep(10);
                bytes_read = send(s, buffer, BUFFER_SIZE, 0);
            }
        }
        // printf("bytes send %d\n", bytes_read);

        new_val = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(pdata->pbar)) + (double)BUFFER_SIZE/(double)pdata->filesize;
        totalsize += temp;
        // printf("%.2f%%\n", new_val*100);
        if(totalsize>=pdata->filesize||bytes_read<=0)
        {
            gtk_widget_destroy(pdata->window);
            CloseHandle(pdata->hFile);
            recv_file_message();
            action();
            gdk_input_remove(pdata->timer);
            return FALSE;
            // return ;
        }
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pdata->pbar), new_val);
        // return ;
        return TRUE;
    }
}

void upload(GtkWidget * widget, gpointer data)
{
    // gtk_widget_set_sensitive((GtkWidget *)data, FALSE);
    GtkWidget * file_chooser_dialog = gtk_file_chooser_dialog_new(_("Upload"),
        (GtkWindow *)data,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        _("Cancel"),
        GTK_RESPONSE_CANCEL,
        _("Open"),
        GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_widget_show(file_chooser_dialog);
    gtk_window_stick(GTK_WINDOW(file_chooser_dialog));
    int res = gtk_dialog_run(GTK_DIALOG(file_chooser_dialog));
    if(res==GTK_RESPONSE_ACCEPT)
    {
        char * filename;
        GtkFileChooser * chooser = GTK_FILE_CHOOSER(file_chooser_dialog);
        filename = gtk_file_chooser_get_filename(chooser);

        ProgressData * pdata = (ProgressData *)g_malloc(sizeof(ProgressData));
        pdata->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_widget_set_size_request(pdata->window, 300, 80);
        gtk_window_set_resizable(GTK_WINDOW(pdata->window), FALSE);
        gtk_window_set_title(GTK_WINDOW(pdata->window), _("Progress"));
        gtk_window_set_position(GTK_WINDOW(pdata->window), GTK_WIN_POS_CENTER);
        gtk_window_set_icon(GTK_WINDOW(pdata->window), gdk_pixbuf_new_from_file("./resources/folder-256x256.png", NULL));
        gtk_container_set_border_width(GTK_CONTAINER(pdata->window), 10);
        g_signal_connect(G_OBJECT(pdata->window), "destroy", G_CALLBACK(destroy_bar), pdata);
        gtk_window_set_modal(GTK_WINDOW(pdata->window), TRUE);

            GtkWidget * vbox = gtk_vbox_new(FALSE, 0);
            gtk_container_add(GTK_CONTAINER(pdata->window), vbox);
            gtk_widget_show(vbox);

                GtkWidget * label = gtk_label_new(_("Upload Progress:"));
                gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 8);
                gtk_widget_show(label);

                pdata->pbar = gtk_progress_bar_new();
                gtk_box_pack_start(GTK_BOX(vbox), pdata->pbar, FALSE, FALSE, 0);
                gtk_widget_show(pdata->pbar);
                gtk_widget_show(pdata->window);
                HANDLE hFile = CreateFile(filename,
                    GENERIC_READ,
                    0,
                    NULL,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
                LARGE_INTEGER fsize;
                GetFileSizeEx(hFile, &fsize);
                // ConvertUTF8ToANSI(filename, pdata->filename);
                // pdata->filename = UTF8ToUnicode(filename);
                unsigned int i;
                for(i=strlen(filename)-1;i>=0;i--)
                {
                    if(filename[i]=='\\')
                    {
                        break;
                    }
                }
                i ++;
                char temp[MAX_PATH] = {0};
                int j = 0;
                for(;i<strlen(filename);i++)
                {
                    temp[j++] = filename[i];
                }
                strcpy(pdata->filename, _U(temp));
                pdata->hFile = hFile;
                pdata->parent = (GtkWidget *)data;
                pdata->filesize = (__int64)fsize.QuadPart;
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "3%s\n%I64d", pdata->filename, pdata->filesize);
                // printf("%I64d\n", pdata->filesize);
                totalsize = 0;
                send(s, buffer, BUFFER_SIZE, 0);
                // pdata->timer = gdk_input_add(s, GDK_INPUT_WRITE, uploading, pdata);
                // pdata->timer = gtk_timeout_add(100, uploading, pdata);
                // gpointer point;// = (gpointer)pdata;
                pdata->timer = gtk_idle_add(uploading, pdata);

        // gtk_timeout_remove(pdata->timer);
        // gtk_widget_destroy(pdata->window);
        // pdata->timer = 0;
        // pdata->window = NULL;
        // g_free(pdata);
        // reacte with the open button
        gtk_widget_destroy(file_chooser_dialog);
        g_free(filename);
    }
    else
    {
        gtk_widget_destroy(file_chooser_dialog);
        // gtk_widget_set_sensitive((GtkWidget *)data, TRUE);
    }
}

typedef struct {
    GtkWidget * widget;
    GtkTreeSelection * select;
} widgetAndSelect;

gboolean right_click_lists(GtkWidget * widget, GdkEventButton *event, gpointer data)
{
    widgetAndSelect * pdata = (widgetAndSelect *)data;
    GtkTreePath * path;
    GtkTreeViewColumn * col;
    GtkTreeModel * model;
    GtkTreeIter iter;
    gint x, y;
    gint cell_x, cell_y;

    if(event->button==1)
    {
        if(event->type==GDK_2BUTTON_PRESS)
        {
            x = (gint)event->x;
            y = (gint)event->y;
            gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(lists), x, y, &path, &col, &cell_x, &cell_y);
            gtk_widget_grab_focus(lists);
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(lists), path, NULL, FALSE);
            gtk_tree_path_free(path);
            char * value;
            int type;
            if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(pdata->select), &model, &iter))
            {
                gtk_tree_model_get(model, &iter, FILENAME, &value, -1);
                gtk_tree_model_get(model, &iter, TYPE, &type, -1);
                if(type==1)
                {
                    memset(buffer, 0, sizeof(buffer));
                    sprintf(buffer, "0%s", value);
                    // printf("cd %s\n", value);
                    send(s, buffer, BUFFER_SIZE, 0);
                    recv_file_message();
                    action();
                    return TRUE;
                }
            }
        }
        return FALSE;
    }
    if(event->button==2)
    {
        return TRUE;
    }
    if(event->button==3)
    {
        if(event->type==GDK_BUTTON_PRESS)
        {
            GdkEventButton * bevent = (GdkEventButton *)event;
            gtk_menu_popup(GTK_MENU(right_click), NULL, NULL, NULL, NULL, bevent->button, bevent->time);
            x = (gint)event->x;
            y = (gint)event->y;
            gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(lists), x, y, &path, &col, &cell_x, &cell_y);
            gtk_widget_grab_focus(lists);
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(lists), path, NULL, FALSE);
            gtk_tree_path_free(path);
            int value;
            if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(pdata->select), &model, &iter))
            {
                gtk_tree_model_get(model, &iter, TYPE, &value, -1);
                if(value==1)
                {
                    gtk_widget_set_sensitive(pdata->widget, FALSE);
                }
                else
                {
                    gtk_widget_set_sensitive(pdata->widget, TRUE);
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

void delete_file(GtkWidget * widget, GtkTreeSelection * select)
{
    GtkTreeIter iter;
    GtkTreeModel * model;
    char * value;
    if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(select), &model, &iter))
    {
        gtk_tree_model_get(model, &iter, FILENAME, &value, -1);
        gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
        // printf("%s was deleted.\n", value);
    }
}

typedef struct {
    GtkTreeSelection * select;
    GtkWindow * window;
} selectAndWindow;

int down_flag = 0;
// void downloading(gpointer data, gint source, GdkInputCondition condition)
gint downloading(gpointer data)
{
    // if(condition==GDK_INPUT_READ)
    // if(down_flag)
    {
        ProgressData * pdata = (ProgressData *)data;
        // gtk_window_stick(GTK_WINDOW(pdata->window));
        gdouble new_val;

        int bytes_written = 0;
        memset(buffer, 0, sizeof(buffer));
        bytes_written = recv(s, buffer, BUFFER_SIZE, 0);
        DWORD temp;
        down_flag = 0;
        long needWrite = 0;
        if(bytes_written<=pdata->filesize-totalsize)
            needWrite = bytes_written;
        else
            needWrite = pdata->filesize - totalsize;

        WriteFile(pdata->hFile, buffer, needWrite, &temp, NULL);
        down_flag = 1;
        // printf("bytes written %ld\n", strlen(buffer));

        if(bytes_written<0)
        {
            // WSACleanup();
            // while(bytes_written<0)
            {
                // Sleep(10);
                // bytes_written = recv(s, buffer, BUFFER_SIZE, 0);
            }
            bytes_written = 0;
        }

        // printf("filesize:%I64d\n", pdata->filesize);
        new_val = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(pdata->pbar)) + (double)bytes_written/Int64ToDouble(pdata->filesize);
        // printf("%.2f%%\n", new_val*100);
        totalsize += bytes_written;
        if(totalsize>=pdata->filesize||bytes_written<=0)
        // if(bytes_written<BUFFER_SIZE)
        {
            gtk_widget_destroy(pdata->window);
            CloseHandle(pdata->hFile);
            gdk_input_remove(pdata->timer);
            // int length = 0;
            /*while(length<BUFFER_SIZE&&flag)
            {
                // printf("hit");
                Sleep(10);
                char temp[BUFFER_SIZE] = {0};
                length += recv(s, temp, BUFFER_SIZE-length, 0);
                if(length<0)
                {
                    length = 0;
                    continue;
                }
            }*/
            // return ;
            return FALSE;
        }
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pdata->pbar), new_val);
        return TRUE;
        // return ;
    }
}

void download(GtkWidget * widget, gpointer data)
{
    selectAndWindow * data_point = (selectAndWindow *)data;

    GtkTreeIter iter;
    GtkTreeModel * model;
    char * value;
    int filesize;
    if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(data_point->select), &model, &iter))
    {
        gtk_tree_model_get(model, &iter, FILENAME, &value, -1);
        gtk_tree_model_get(model, &iter, FILEBYTES, &filesize, -1);
        // printf("%s (%d bytes) is downloading.\n", value, filesize);

        GtkWidget * file_chooser_dialog = gtk_file_chooser_dialog_new(_("Download"),
            data_point->window,
            GTK_FILE_CHOOSER_ACTION_SAVE,
            _("Cancel"),
            GTK_RESPONSE_CANCEL,
            _("Save"),
            GTK_RESPONSE_ACCEPT,
            NULL);
        gtk_widget_show(file_chooser_dialog);
        gtk_window_stick(GTK_WINDOW(file_chooser_dialog));
        GtkFileChooser * chooser = GTK_FILE_CHOOSER(file_chooser_dialog);
        gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
        gtk_file_chooser_set_current_name(chooser, value);
        int res = gtk_dialog_run(GTK_DIALOG (file_chooser_dialog));
        if(res==GTK_RESPONSE_ACCEPT)
        {
            char * filename;
            filename = gtk_file_chooser_get_filename(chooser);

            ProgressData * pdata = (ProgressData *)g_malloc(sizeof(ProgressData));
            // pdata->filesize = (double)filesize;
            pdata->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            gtk_widget_set_size_request(pdata->window, 300, 80);
            gtk_window_set_resizable(GTK_WINDOW(pdata->window), FALSE);
            gtk_window_set_title(GTK_WINDOW(pdata->window), _("Progress"));
            gtk_window_set_position(GTK_WINDOW(pdata->window), GTK_WIN_POS_CENTER);
            gtk_window_set_icon(GTK_WINDOW(pdata->window), gdk_pixbuf_new_from_file("./resources/folder-256x256.png", NULL));
            gtk_container_set_border_width(GTK_CONTAINER(pdata->window), 10);
            g_signal_connect(G_OBJECT(pdata->window), "destroy", G_CALLBACK(destroy_bar), pdata);
            gtk_window_set_modal(GTK_WINDOW(pdata->window), TRUE);

                GtkWidget * vbox = gtk_vbox_new(FALSE, 0);
                gtk_container_add(GTK_CONTAINER(pdata->window), vbox);
                gtk_widget_show(vbox);

                    GtkWidget * label = gtk_label_new(_("Download Progress:"));
                    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 8);
                    gtk_widget_show(label);

                    pdata->pbar = gtk_progress_bar_new();
                    gtk_box_pack_start(GTK_BOX(vbox), pdata->pbar, FALSE, FALSE, 0);
                    gtk_widget_show(pdata->pbar);
                    gtk_widget_show(pdata->window);
                    HANDLE hFile = CreateFile(_U(filename),
                        GENERIC_WRITE|GENERIC_READ,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
                    pdata->hFile = hFile;
                    // pdata->filesize = filesize;
                    for(int i=0;i<file_count;i++)
                    {
                        if(strcmp(value, files[i].cFileName)==0)
                        {
                            // printf("find!\n");
                            pdata->filesize = files[i].file_total;
                            break;
                        }
                    }
                    // ConvertUTF8ToANSI(filename, pdata->filename);
                    // pdata->filename = UTF8ToUnicode(filename);
                    strcpy(pdata->filename, value);
                    memset(buffer, 0, sizeof(buffer));
                    sprintf(buffer, "1%s", value);
                    send(s, buffer, BUFFER_SIZE, 0);
                    totalsize = 0;
                    pdata->parent = GTK_WIDGET(data_point->window);
                    // pdata->timer = gdk_input_add(s, GDK_INPUT_READ, downloading, pdata);
                    // pdata->timer = gtk_timeout_add(100, downloading, pdata);
                    down_flag = 1;
                    pdata->timer = gtk_idle_add(downloading, pdata);

            g_free(filename);
        }
        gtk_widget_destroy(file_chooser_dialog);
    }
}

void help(GtkWidget * widget, gpointer data)
{
    // GtkWidget * help_dia = gtk_about_dialog_new();
    // char author[5][20] = {"Ning Xiaodong", "Qian Jun", "Ding Jiarui", "Wang Yu", NULL};
    GdkPixbuf * logo = gdk_pixbuf_new_from_file(_("./resources/folder-128x128.png"), NULL);
    gtk_show_about_dialog((GtkWindow *)data,
        "logo",         logo,
        "program-name", "Remote File Manager",
        "version",      "v1.0",
        // "authors",      author,
        // "website",      "http://weibo.com/2211959137/profile?rightmod=1&amp;wvr=5&amp;mod=personinfo",
        // "website-label","go to the author's website",
        "comments",     "upload: click the upload button in the toolbox.\ndownload: right click the file in the list, then choose download.\nnewfolder: click the newfolder button in the toolbox.\nback: click the back button in the toolbox to browse the files in the upper directory.\ndelete: right click the file in the list, then choose delete.",
        "copyright",    "BIT CS School OS lecture Work Team",
        // "logo-icon_name","help",
        "title",        _("Help"),
        NULL);
    // g_free(author);
    if(logo)
    {
        g_object_unref(logo);
    }
}

void up()
{
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "back");
    // printf("cd..\n");
    send(s, buffer, BUFFER_SIZE, 0);
    recv_file_message();
    action();
}

void close_new_folder(GtkWidget * widget, gpointer data)
{
    gtk_widget_destroy((GtkWidget *)data);
}

typedef struct _tempStruc {
    GtkEntry * Entry;
    GtkWindow * Window;
} entryAndWindow;

void new_folder(GtkWidget * widget, gpointer data)
{
    entryAndWindow * pdata = (entryAndWindow *)data;
    const char * temp = gtk_entry_get_text(pdata->Entry);
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "2%s", temp);
    // printf("new folder : %s\n", temp);
    send(s, buffer, BUFFER_SIZE, 0);
    gtk_widget_destroy(GTK_WIDGET(pdata->Window));
    recv_file_message();
    action();
}

void show_new_folder()
{
    GtkWidget * window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request(window, 340, 135);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(window), _("new folder"));
    gtk_window_set_icon(GTK_WINDOW(window), gdk_pixbuf_new_from_file("./resources/folder-256x256.png", NULL));
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_modal(GTK_WINDOW(window), TRUE);
    gtk_widget_show(window);

        GtkWidget * hbox_main = gtk_hbox_new(0, 0);
        gtk_container_add(GTK_CONTAINER(window), hbox_main);
        gtk_widget_show(hbox_main);

            GtkWidget * image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_file("./resources/folder-new-100x100.png", NULL));
            gtk_box_pack_start(GTK_BOX(hbox_main), image, FALSE, FALSE, 0);
            // g_object_unref(image);
            gtk_widget_show(image);

            GtkWidget * vbox = gtk_vbox_new(0, 0);
            gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
            gtk_box_pack_start(GTK_BOX(hbox_main), vbox, FALSE, FALSE, 15);
            gtk_widget_show(vbox);

                GtkWidget * hbox = gtk_vbox_new(0, 0);
                gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
                gtk_widget_show(hbox);

                    GtkWidget * label = gtk_label_new(_("Please enter the new folder's name:"));
                    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
                    gtk_widget_show(label);

                    GtkWidget * entry = gtk_entry_new();
                    gtk_entry_set_max_length(GTK_ENTRY(entry), 255);
                    gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 5);
                    gtk_widget_show(entry);

                GtkWidget * bbox = gtk_hbox_new(0, 0);
                gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 5);
                gtk_widget_show(bbox);

                    GtkWidget * button_new = gtk_button_new_with_label(_("Cancel"));
                    gtk_widget_set_size_request(button_new, 50, 25);
                    gtk_box_pack_end(GTK_BOX(bbox), button_new, FALSE, FALSE, 0);
                    g_signal_connect(G_OBJECT(button_new), "clicked", G_CALLBACK(close_new_folder), (gpointer)window);
                    gtk_widget_show(button_new);

                    GtkWidget * button_OK = gtk_button_new_with_label(_("OK"));
                    gtk_widget_set_size_request(button_OK, 50, 25);
                    gtk_box_pack_end(GTK_BOX(bbox), button_OK, FALSE, FALSE, 5);
                    entryAndWindow * temp = (entryAndWindow *)g_malloc(sizeof(entryAndWindow));
                    temp->Entry = GTK_ENTRY(entry);
                    temp->Window = GTK_WINDOW(window);
                    g_signal_connect(G_OBJECT(button_OK), "clicked", G_CALLBACK(new_folder), (gpointer)temp);
                    gtk_widget_show(button_OK);
}

GtkWidget * create_main_window()
{
    GtkWidget * window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(window), _("Remote File Manager"));
    gtk_window_set_icon(GTK_WINDOW(window), gdk_pixbuf_new_from_file("./resources/folder-256x256.png", NULL));
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(main_quit), NULL);

        GtkWidget * vbox = gtk_vbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(window), vbox);
        gtk_widget_show(vbox);

            GtkWidget * bbox = gtk_hbox_new(FALSE, 0);
            gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 10);
            gtk_widget_show(bbox);

                GtkWidget * button_upload = gtk_button_new_with_label(_("Upload"));
                gtk_widget_show(button_upload);
                gtk_widget_set_size_request(button_upload, 60, 30);
                gtk_box_pack_start(GTK_BOX(bbox), button_upload, FALSE, FALSE, 10);
                gtk_signal_connect(GTK_OBJECT(button_upload), "clicked", G_CALLBACK(upload), (gpointer)window);

                GtkWidget * button_folder = gtk_button_new();
                GtkWidget * image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_file("./resources/folder-new.png", NULL));
                // g_object_unref(image);
                gtk_widget_show(button_folder);
                gtk_widget_show(image);
                gtk_widget_set_size_request(button_folder, 35, 30);
                gtk_container_add(GTK_CONTAINER(button_folder), image);
                gtk_box_pack_start(GTK_BOX(bbox), button_folder, FALSE, FALSE, 0);
                gtk_signal_connect(GTK_OBJECT(button_folder), "clicked", G_CALLBACK(show_new_folder), NULL);

                GtkWidget * button_up = gtk_button_new();
                image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_file("./resources/go-previous.png", NULL));
                // g_object_unref(image);
                gtk_widget_show(button_up);
                gtk_widget_show(image);
                gtk_widget_set_size_request(button_up, 30, 30);
                gtk_container_add(GTK_CONTAINER(button_up), image);
                gtk_box_pack_start(GTK_BOX(bbox), button_up, FALSE, FALSE, 10);
                gtk_signal_connect(GTK_OBJECT(button_up), "clicked", G_CALLBACK(up), NULL);

                GtkWidget * button_help = gtk_button_new();
                image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_file("./resources/help-faq.png", NULL));
                // g_object_unref(image);
                gtk_widget_show(button_help);
                gtk_widget_show(image);
                gtk_widget_set_size_request(button_help, 30, 30);
                gtk_container_add(GTK_CONTAINER(button_help), image);
                gtk_box_pack_start(GTK_BOX(bbox), button_help, FALSE, FALSE, 0);
                gtk_signal_connect(GTK_OBJECT(button_help), "clicked", G_CALLBACK(help), (gpointer)window);

            GtkWidget * scrolled = gtk_scrolled_window_new(NULL, NULL);
            GtkWidget * hbox = gtk_hbox_new(FALSE ,0);
            gtk_widget_show(hbox);
            gtk_box_pack_start(GTK_BOX(hbox), scrolled, TRUE, TRUE, 10);
            gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
            gtk_widget_show(scrolled);
            gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_ETCHED_IN);
            gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

                store = gtk_list_store_new(COLUMN_N, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);
                lists = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
                gtk_widget_show(lists);
                gtk_container_add(GTK_CONTAINER(scrolled), lists);
                GtkTreeSelection * select = gtk_tree_view_get_selection(GTK_TREE_VIEW(lists));
                gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);

                    GtkTreeViewColumn * column;
                    GtkCellRenderer * renderer;

                    renderer = gtk_cell_renderer_pixbuf_new();
                    column = gtk_tree_view_column_new_with_attributes("", renderer, "pixbuf", ICON, NULL);
                    gtk_cell_renderer_set_fixed_size(renderer, 18, 18);
                    gtk_tree_view_append_column(GTK_TREE_VIEW(lists), column);

                    renderer = gtk_cell_renderer_text_new();
                    column = gtk_tree_view_column_new_with_attributes("                      name                      ",
                        renderer, "text", FILENAME, NULL);
                    gtk_tree_view_append_column(GTK_TREE_VIEW(lists), column);
                    gtk_cell_renderer_set_fixed_size(renderer, -1, 18);

                    renderer = gtk_cell_renderer_text_new();
                    column = gtk_tree_view_column_new_with_attributes(_("      file size      "), renderer, "text", FILESIZE, NULL);
                    gtk_tree_view_append_column(GTK_TREE_VIEW(lists), column);
                    gtk_cell_renderer_set_fixed_size(renderer, -1, 18);

                    renderer = gtk_cell_renderer_text_new();
                    column = gtk_tree_view_column_new_with_attributes(_("      last update      "), renderer, "text", LASTUPDATE, NULL);
                    gtk_tree_view_append_column(GTK_TREE_VIEW(lists), column);
                    gtk_cell_renderer_set_fixed_size(renderer, -1, 18);

                    renderer = gtk_cell_renderer_text_new();
                    column = gtk_tree_view_column_new_with_attributes(_("file type"), renderer, "text", TYPE, NULL);

                    renderer = gtk_cell_renderer_text_new();
                    column = gtk_tree_view_column_new_with_attributes(_("file bytes"), renderer, "text", FILEBYTES, NULL);

                        right_click = GTK_MENU_SHELL(gtk_menu_new());
                        GtkWidget * right_click_item_download = gtk_menu_item_new_with_label(_("download"));
                        gtk_menu_shell_append(GTK_MENU_SHELL(right_click), right_click_item_download);
                        gtk_widget_show(right_click_item_download);

                        GtkWidget * sep = gtk_separator_menu_item_new();
                        gtk_menu_shell_append(GTK_MENU_SHELL(right_click), sep);
                        gtk_widget_show(sep);

                        GtkWidget * right_click_item_delete = gtk_menu_item_new_with_label(_("delete"));
                        gtk_menu_shell_append(GTK_MENU_SHELL(right_click), right_click_item_delete);
                        gtk_widget_show(right_click_item_delete);

                        widgetAndSelect * pdata2 = (widgetAndSelect *)g_malloc(sizeof(widgetAndSelect));
                        pdata2->widget = right_click_item_download;
                        pdata2->select = select;
                        g_signal_connect(GTK_TREE_VIEW(lists), "button_press_event", G_CALLBACK(right_click_lists), pdata2);
                        selectAndWindow * pdata = (selectAndWindow *)g_malloc(sizeof(selectAndWindow));
                        pdata->window = GTK_WINDOW(window);
                        pdata->select = select;
                        g_signal_connect(G_OBJECT(right_click_item_download), "activate", G_CALLBACK(download), pdata);
                        g_signal_connect(G_OBJECT(right_click_item_delete), "activate", G_CALLBACK(delete_file), select);

            GtkWidget * invisible = gtk_scrolled_window_new(NULL, NULL);
            gtk_widget_show(invisible);
            gtk_box_pack_start(GTK_BOX(vbox), invisible, FALSE, FALSE, 5);
            gtk_widget_set_size_request(invisible, 0, 0);
            gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(invisible), GTK_POLICY_NEVER, GTK_POLICY_NEVER);

    return window;
}

GtkWidget * show_start_window()
{
    GtkWidget * window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request(window, 320, 135);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(window), _("Remote File Manager"));
    gtk_window_set_icon(GTK_WINDOW(window), gdk_pixbuf_new_from_file("./resources/folder-256x256.png", NULL));
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(main_quit), NULL);
    gtk_widget_show(window);

        GtkWidget * hbox_main = gtk_hbox_new(0, 0);
        gtk_container_add(GTK_CONTAINER(window), hbox_main);
        gtk_widget_show(hbox_main);

            GtkWidget * image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_file("./resources/preferences-system-sharing.png", NULL));
            gtk_box_pack_start(GTK_BOX(hbox_main), image, FALSE, FALSE, 0);
            // g_object_unref(image);
            gtk_widget_show(image);

            GtkWidget * vbox = gtk_vbox_new(0, 0);
            gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
            gtk_box_pack_start(GTK_BOX(hbox_main), vbox, FALSE, FALSE, 15);
            gtk_widget_show(vbox);

                GtkWidget * hbox = gtk_vbox_new(0, 0);
                gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
                gtk_widget_show(hbox);

                    GtkWidget * label = gtk_label_new(_("Please enter the server's IP:"));
                    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
                    gtk_widget_show(label);

                    GtkWidget * entry = gtk_entry_new();
                    gtk_entry_set_max_length(GTK_ENTRY(entry), 20);
                    gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 5);
                    gtk_widget_show(entry);

                GtkWidget * bbox = gtk_hbox_new(0, 0);
                gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 5);
                gtk_widget_show(bbox);

                    GtkWidget * button_QUIT = gtk_button_new_with_label(_("Quit"));
                    gtk_widget_set_size_request(button_QUIT, 50, 25);
                    gtk_box_pack_end(GTK_BOX(bbox), button_QUIT, FALSE, FALSE, 0);
                    g_signal_connect(G_OBJECT(button_QUIT), "clicked", G_CALLBACK(main_quit), NULL);
                    gtk_widget_show(button_QUIT);

                    GtkWidget * button_OK = gtk_button_new_with_label(_("OK"));
                    gtk_widget_set_size_request(button_OK, 50, 25);
                    gtk_box_pack_end(GTK_BOX(bbox), button_OK, FALSE, FALSE, 5);
                    entryAndWindow * data = (entryAndWindow *)g_malloc(sizeof(entryAndWindow));;
                    data->Entry = GTK_ENTRY(entry);
                    data->Window = GTK_WINDOW(window);
                    g_signal_connect(G_OBJECT(button_OK), "clicked", G_CALLBACK(show_main_window), data);
                    gtk_widget_show(button_OK);

    return window;
}

void show_main_window(GtkWidget * widget, gpointer data)
{
    entryAndWindow * data_get = (entryAndWindow *)data;
    // gtk_widget_hide(GTK_WIDGET(data_get->Window));
    const char * IP = gtk_entry_get_text(data_get->Entry);
    if(set_up_socket(IP))
    {
        recv_file_message();
        GtkWidget * window = create_main_window();
        action();
        gtk_widget_hide(GTK_WIDGET(data_get->Window));
        gtk_widget_show(window);
    }
    else
    {
        GtkWidget * message_dia = gtk_message_dialog_new(data_get->Window,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            _("Can not connect to the\n   target IP : %s"), IP);
        gtk_window_set_position(GTK_WINDOW(message_dia), GTK_WIN_POS_CENTER);
        gtk_dialog_run(GTK_DIALOG(message_dia));
        gtk_widget_destroy(message_dia);
        // show_start_window();
        // g_free(data_get->Window);
        // gtk_widget_destroy(GTK_WIDGET(data_get->Window));
        // gtk_widget_show(GTK_WIDGET(data_get->Window));
        // gtk_widget_set_sensitive(GTK_WIDGET(data_get->Window), TRUE);
        // gtk_widget_grab_focus(GTK_WIDGET(data_get->Entry));
    }
}

int main(int argc, char * argv[])
{
    // file_walk();

    gtk_init(&argc, &argv);

    show_start_window();

    gtk_main();
    return 0;
}
