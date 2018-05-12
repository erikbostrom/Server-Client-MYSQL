/********************************************************************
client.c

Utlizing the socket library and GTK+3 GUI.
Communicates with the server (server.c) through port 8080.

Compiles under gcc with the following command:
gcc `pkg-config --cflags gtk+-3.0` -o client client.c `pkg-config 
      --libs gtk+-3.0`


Erik Boström, 2018-04-27
*********************************************************************/
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <gtk/gtk.h>

#define PORT 8080
#define SIZE 10000000
#define QSIZE 1024
#define SSIZE 100
#define XWIN 600
#define YWIN 400

// Global variables
GtkWidget *list;
GtkTreeViewColumn  *column;
gint ncols;


// Call-back function as the GUI button is pressed.
void communication(GtkWidget *widget, gpointer entry)
{  
  GtkListStore       *store;
  GtkTreeIter        iter;
  GtkCellRenderer    *renderer;
  GtkTreeModel       *model;
  GtkWidget          *view;
  GtkTreeSelection   *selection; 
  
  struct sockaddr_in address;
  struct sockaddr_in serv_addr;
  
  int i, j, k, j_buffer, nr_cols, count;
  int str_index[SSIZE];
  int domain     = AF_INET;                          // Use the IPv4 protocol (=2)
  int type       = SOCK_STREAM;                      // Transport layer protocol: TCP (=1)
  int protocol   = 0;                                // Let kernel decide the default protocol to use (=0)
  int client_fd  = socket(domain, type, protocol);   // Return server socket file desctiptor
  
  char *buffer = malloc(sizeof(char) * SIZE);
  char query[QSIZE];
  char subbuff[SSIZE];
  char col_name[SSIZE]={0}; 
  char string[SSIZE]={0};   

  const gchar *str = gtk_entry_get_text(entry);

  
  strncpy(query,str,QSIZE);
  gtk_entry_set_text(entry, "");  // Clear text entry

  printf("%s\n",str);
  
  //
  // Create client socket file descriptor
  //
  if (client_fd < 0)
    {
      printf("client.c: socket failed!\n"); 
      exit(EXIT_FAILURE);
    }
    
  //
  // Preallocate a block of memory for the server address
  //
  memset(&serv_addr, '0', sizeof(serv_addr));

  //
  // Convert IPv4 address from text to binary form
  //
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  int conv_to_bin = inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
  if(conv_to_bin<=0)
    {
      printf("client.c: inet_pton failed!");
      exit(EXIT_FAILURE);
    }

  //
  // Connect to the server
  //
  int connection_nr = connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (connection_nr < 0)
    {
      printf("client.c: Connection to server failed!\n");
      exit(EXIT_FAILURE);
    }

  // Communication with server
  send(client_fd , query , strlen(query) , 0 );  // Send message to server      
  memset(buffer, 0, sizeof(char) * SIZE);
  read(client_fd, buffer, SIZE-1);  // Recieve message from server

  
  // Sort data
  j=0; k=0; nr_cols=1;
  while(!(buffer[j]==';'))
    {
      if(buffer[j]==':')
	{
	  memset(col_name, 0, SSIZE);
	  k=0;
	  
	  str_index[nr_cols-1] = j;
	  nr_cols++;
	}
      else
	{
	  col_name[k] = buffer[j];
	  k++;
	}
      j++;
    }
  str_index[nr_cols-1]=j;
  j_buffer=j;

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), TRUE);
  store = gtk_list_store_new(6, G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,
			     G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
  gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));
  g_object_unref(store);
  

  // Remove columns from previous search
  if (ncols>0)
    {
      for(i=0; i<ncols; i++)
	gtk_tree_view_remove_column ( GTK_TREE_VIEW(list), gtk_tree_view_get_column(GTK_TREE_VIEW(list),0) );
    }
  
  for(i=0; i<nr_cols; i++)
    {
      if(i==0)
	{
	  memset( subbuff, 0, SSIZE);
	  memcpy( subbuff, &buffer[0], str_index[0]);
	  subbuff[str_index[0]]='\0';
	}      
      else
	{
	  memset( subbuff, 0, SSIZE);
	  memcpy( subbuff, &buffer[str_index[i-1]+1], str_index[i]-str_index[i-1]-1);
	  subbuff[str_index[i]-str_index[i-1]-1]='\0';
	}
      renderer = gtk_cell_renderer_text_new();
      column = gtk_tree_view_column_new_with_attributes(subbuff,renderer, "text", i, NULL);
      ncols = gtk_tree_view_insert_column(GTK_TREE_VIEW(list), column,-1);
    }
  

  k=0; j=j_buffer; count=0;
  while(!(buffer[j]==' ' && buffer[j+1]==' '))
    {
      if(buffer[j]==';') // end of line
	{
	  gtk_list_store_set(store, &iter, count, string, -1);
	  memset(string, 0, SSIZE);
	  gtk_list_store_append(store, &iter);
	  k=0;
	  count=0;
	}
      else if(buffer[j]==',') // end of column
	{
	  gtk_list_store_set(store, &iter, count, string, -1);
	  memset(string, 0, SSIZE);
	  k=0;
	  count++;
	}
      else
	{
	  string[k] = buffer[j];
	  k++;
	}      
      j++;
    }
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
  close(client_fd);
  free(buffer);
}


  
int main(int argc, char **argv)
{
  
  GtkWidget *window;
  GtkWidget *sw;
  GtkWidget *add;
  GtkWidget *label;  
  GtkWidget *entry;
  GtkWidget *vbox;
  GtkWidget *hbox;

  
  // Initialize GTK
  gtk_init(&argc, &argv);
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "MySQL my_database @ server");
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER (window), 10);
  gtk_widget_set_size_request(window, XWIN, YWIN);  

  // A list widget in a scrollable window
  sw = gtk_scrolled_window_new(NULL, NULL);
  list = gtk_tree_view_new();
  gtk_container_add(GTK_CONTAINER(sw), list);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);

  // A tree view is placed inside the scrollable window
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);

  // Buttons
  add = gtk_button_new_with_label("Search");

  // Text entry
  entry = gtk_entry_new();
  gtk_widget_set_size_request(entry, XWIN-200, -1);

  // Boxes and locations
  vbox = gtk_vbox_new(FALSE, 0); // vertical container box
  hbox = gtk_hbox_new(FALSE, 5); // horizontal container box

  label = gtk_label_new("Enter SQL query:");
  gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 5);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, TRUE, 3);
  gtk_box_pack_start(GTK_BOX(hbox), add, FALSE, TRUE, 3);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 3);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  // If the OK button is pressed the message should be sent to server
  g_signal_connect(G_OBJECT(add), "clicked", G_CALLBACK(communication), entry);  
  
  // Close window
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

  gtk_widget_show_all(window);
    
  gtk_main ();
  return 0;

}
