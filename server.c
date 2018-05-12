/********************************************************************
server.c

Utlizing the socket library.
Reads MySQL database. Listens at port 8080.

A MySQL database "my_database" must created that include a table
including the content of the data.csv file.

Compiles with the flags: `mysql_config --cflags --libs`.

Command line inputs:
argv[1] = [MySQL username]
argv[2] = [MySQL password]

Erik Boström, 2018-04-27
*********************************************************************/
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h> 
#include <string.h>
#include <my_global.h>
#include <mysql.h>

#define PORT 8080
#define SIZE 10000000
#define QSIZE 1024

int main(int argc, char const *argv[])
{
  char buffer[QSIZE] = {0};
  char header_str[QSIZE] = {0};
  char *str = malloc(sizeof(char) * SIZE);

  int i, set_server, bind_server_to_address, max_client_connections;
  int wait_for_client, addrlen, client_fd, num_fields;

  int domain     = AF_INET;                          // Use the IPv4 protocol (=2)
  int type       = SOCK_STREAM;                      // Transport layer protocol: TCP (=1)
  int protocol   = 0;                                // Let kernel decide the default protocol to use (=0)
  int server_fd  = socket(domain, type, protocol);   // Return server socket file desctiptor
  int opt        = 1;

  struct sockaddr_in address;

  MYSQL_FIELD *fields;
  MYSQL_RES *result;
  MYSQL_ROW row;
  MYSQL *con;

  // Connection to MySQL
  con = mysql_init(NULL);
  if (con == NULL) 
    {
      fprintf(stderr, "server.c: %s\n", mysql_error(con));
      exit(EXIT_FAILURE);
    }
  
  //if (mysql_real_connect(con,"localhost", argv[1], argv[2], NULL, 0, NULL, 0) == NULL)
  if (mysql_real_connect(con,"localhost","root","zorro2007",NULL,0,NULL,0) == NULL) 
    {
      fprintf(stderr, "server.c: %s\n", mysql_error(con));
      mysql_close(con);
      exit(EXIT_FAILURE);
    }
  
  if (mysql_query(con, "USE my_database"))
    {
      fprintf(stderr, "server.c: %s\n", mysql_error(con));
      mysql_close(con);
      exit(EXIT_FAILURE);
    }
  

  // Create server socket file descriptor
  if (server_fd == 0)
    {
      perror("server.c: socket failed!");
      exit(EXIT_FAILURE);
    }

  
  // Enforce an unique address for the server
  set_server = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
  if (set_server!=0)
    {
      perror("server.c: setsockopt failed!");
      exit(EXIT_FAILURE);
    }
  

  // Bind the server to an address and the specified port number. 
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons( PORT );  
  bind_server_to_address = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
  if (bind_server_to_address < 0)
    {
      perror("server.c: bind failed!");
      exit(EXIT_FAILURE);
    }


  // Put the server socket in a passive mode and wait for the client to make a connection.
  max_client_connections = 3;
  wait_for_client = listen(server_fd, max_client_connections);
  if (wait_for_client < 0)
    {
      perror("server.c: listen failed!");
      exit(EXIT_FAILURE);
    }

  
  // Infinite loop
  while(1)
    {
  
      // Get a file descriptor corresponding to the connected socket
      // Accept is an infinite loop, the server is always running (may eat CPU...).
      addrlen = sizeof(address);
      client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
      if (client_fd<0)
	{
	  perror("server.c: accept failed!");
	  exit(EXIT_FAILURE);
	}      

      // Read SQL Query message from client
      memset(buffer, 0, QSIZE);
      read(client_fd , buffer, QSIZE);


      // Fetch the recieved query from MySQL database
      if(mysql_query(con, buffer))
	{
	  fprintf(stderr, "server.c: %s\n", mysql_error(con));
	  mysql_close(con);
	  exit(EXIT_FAILURE);
	}

      // Get the result set for to the MySQL query.
      result = mysql_store_result(con);

  
      // Get the number of columns for the result
      num_fields = mysql_num_fields(result);

      
      // Get column names for the result set
      fields = mysql_fetch_fields(result);
      memset(header_str, 0, sizeof header_str);
      for(i = 0; i < num_fields; i++)
	{
	  if(i>0) strcat(header_str, ":");
	  strcat(header_str,fields[i].name);
	}
      header_str[strlen(header_str)] = ';';
      
      
      // Send fetched data back to client
      memset(str, 0, sizeof str);
      strcat(str,header_str);      
      while ((row = mysql_fetch_row(result)))
	{
	  for(i = 0; i < num_fields; i++)
	    {
	      strcat(str,row[i] ? row[i] : "NULL");
	      if(i<num_fields-1)
		strcat(str, ",");
	      else
		strcat(str, ";");
	    }
	}
      strcat(str, "          ");
      send(client_fd , str , strlen(str) , 0 );
      close(client_fd);
    }
  
  mysql_close(con);
  free(str);
  return 0;
}
