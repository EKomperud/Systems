/*
 * friendlist.c - [Starting code for] a web-based friend-graph manager.
 *
 * Based on:
 *  tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *      GET method to serve static and dynamic content.
 *   Tiny Web server
 *   Dave O'Hallaron
 *   Carnegie Mellon University
 */
#include "csapp.h"
#include "dictionary.h"
#include "more_string.h"

static void doit(int fd);
static void *t_doit(void *connfdp);
static dictionary_t *read_requesthdrs(rio_t *rp);
static void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *d);
static void clienterror(int fd, char *cause, char *errnum, 
                        char *shortmsg, char *longmsg);
static void print_stringdictionary(dictionary_t *d);
static void serve_request(int fd, dictionary_t *query);
static void serve_sum(int fd, dictionary_t *query);
static void serve_friends(int fd, dictionary_t *query);
static void serve_befriend(int fd, dictionary_t *query);
static void serve_unfriend(int fd, dictionary_t *query);
static void serve_introduce(int fd, dictionary_t *query);

static dictionary_t *users;

int main(int argc, char **argv) 
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);

  /* Don't kill the server if there's an error, because
     we want to survive errors due to a client. But we
     do want to report errors. */
  exit_on_error(0);

  /* Also, don't stop on broken connections: */
  Signal(SIGPIPE, SIG_IGN);

  /* Create the friends dictionary */
  users = make_dictionary(1,free);

  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    if (connfd >= 0) {
      Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                  port, MAXLINE, 0);
      printf("Accepted connection from (%s, %s)\n", hostname, port);

      int *connfdp;
      pthread_t th;
      connfdp = malloc(sizeof(int));
      *connfdp = connfd;
      Pthread_create(&th, NULL, t_doit, connfdp);
      Pthread_detach(th);
    }
  }
}

void *t_doit(void *connfdp)
{
  int connfd = *(int *)connfdp;
  free(connfdp);
  doit(connfd);
  close(connfd);
  return NULL;
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd) 
{
  char buf[MAXLINE], *method, *uri, *version;
  rio_t rio;
  dictionary_t *headers, *query;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
    return;
  printf("%s", buf);
  
  if (!parse_request_line(buf, &method, &uri, &version)) {
    clienterror(fd, method, "400", "Bad Request",
                "Friendlist did not recognize the request");
  } else {
    if (strcasecmp(version, "HTTP/1.0")
        && strcasecmp(version, "HTTP/1.1")) {
      clienterror(fd, version, "501", "Not Implemented",
                  "Friendlist does not implement that version");
    } else if (strcasecmp(method, "GET")
               && strcasecmp(method, "POST")) {
      clienterror(fd, method, "501", "Not Implemented",
                  "Friendlist does not implement that method");
    } else {
      headers = read_requesthdrs(&rio);

      /* Parse all query arguments into a dictionary */
      query = make_dictionary(COMPARE_CASE_SENS, free);
      parse_uriquery(uri, query);
      if (!strcasecmp(method, "POST"))
        read_postquery(&rio, headers, query);

      /* For debugging, print the dictionary */
      print_stringdictionary(query);

      /* You'll want to handle different queries here,
         but the intial implementation always returns
         nothing: */
      printf("uri: %s \n", uri);
      printf("starts with /friends? %d \n", starts_with("/friends",uri));
      if (starts_with("/sum",uri))
      	serve_sum(fd, query);
      else if (starts_with("/friends",uri))
      	serve_friends(fd, query);
      else if (starts_with("/befriend",uri))
      	serve_befriend(fd, query);
      else if (starts_with("/unfriend",uri))
      	serve_unfriend(fd,query);
      else
      {
      	serve_request(fd, query);
      }

      /* Clean up */
      free_dictionary(query);
      free_dictionary(headers);
    }

    /* Clean up status line */
    free(method);
    free(uri);
    free(version);
  }
}

/*
 * read_requesthdrs - read HTTP request headers
 */
dictionary_t *read_requesthdrs(rio_t *rp) 
{
  char buf[MAXLINE];
  dictionary_t *d = make_dictionary(COMPARE_CASE_INSENS, free);

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    parse_header_line(buf, d);
  }
  
  return d;
}

void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *dest)
{
  char *len_str, *type, *buffer;
  int len;
  
  len_str = dictionary_get(headers, "Content-Length");
  len = (len_str ? atoi(len_str) : 0);

  type = dictionary_get(headers, "Content-Type");
  
  buffer = malloc(len+1);
  Rio_readnb(rp, buffer, len);
  buffer[len] = 0;

  if (!strcasecmp(type, "application/x-www-form-urlencoded")) {
    parse_query(buffer, dest);
  }

  free(buffer);
}

static char *ok_header(size_t len, const char *content_type) {
  char *len_str, *header;
  
  header = append_strings("HTTP/1.0 200 OK\r\n",
                          "Server: Friendlist Web Server\r\n",
                          "Connection: close\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n",
                          "Content-type: ", content_type, "\r\n\r\n",
                          NULL);
  free(len_str);

  return header;
}

/*
 * serve_request - example request handler
 */
static void serve_request(int fd, dictionary_t *query)
{
  printf("serve request\n");
  size_t len;
  char *body, *header;

  body = strdup("alice\nbob");

  len = strlen(body);

  /* Send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  //free(body);
}

/*
 * serve_sum - example request handler
 */
static void serve_sum(int fd, dictionary_t *query)
{
  printf("serve sum\n");
  size_t len;
  char *body, *header, *x, *y, *sum;

  x = dictionary_get(query, "x");
  y = dictionary_get(query, "y");
  if (!x || !y)
  {
  	clienterror(fd, "?", "400", "Bad Request", "Please provide two numbered arguments");
  	return;
  }

  Sleep(10);
  sum = to_string(atoi(x) + atoi(y));
  body = append_strings(sum, "\n", NULL);
  free(sum);

  len = strlen(body);

  /* send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  //free(body);
}

/*
 * serve_friends - reports the friends of a user in the query
 */
static void serve_friends(int fd, dictionary_t *query)
{
  printf("serve friends\n");
  size_t len;
  char *body, *header, *user;

  user = dictionary_get(query, "user");
  if (!user)
  {
  	clienterror(fd, "?", "400", "Bad Request", "Please provide a user");
  	return;
  }

  dictionary_t *friends = (dictionary_t *)dictionary_get(users, user);
  if (friends != NULL)
  {
    const char **friends_list = dictionary_keys(friends);
    body = join_strings(friends_list, '\n');

    free(friends_list);
  }
  else
  {
  	dictionary_t *new_friends = make_dictionary(1,free);
  	dictionary_set(users, user, new_friends);
  	body = "";
  }
  free(friends);


  len = strlen(body);

  /* send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  //free(body);
}

/*
 * serve_befriend - friends some users and reports the friends of a user defined in the query
 */
static void serve_befriend(int fd, dictionary_t *query)
{
  printf("serve befreind\n");
  size_t len;
  char *body, *header, *user, *friends;

  user = (char *)dictionary_get(query, "user");
  friends = (char *)dictionary_get(query, "friends");
  if (!user || !friends)
  {
  	clienterror(fd, "?", "400", "Bad Request", "Please provide two valid arguments");
  	return;
  }

  dictionary_t *userz_friends = (dictionary_t *)dictionary_get(users, user);
  if (userz_friends == NULL)
  {
  	dictionary_t *new_user = make_dictionary(1,free);
  	dictionary_set(users, user, new_user);
  	userz_friends = (dictionary_t *)dictionary_get(users, user);
  }

  char **new_friends = split_string(friends, '\n');
  int i;
  for(i = 0; new_friends[i] != NULL; i++)
  {
  	dictionary_set(userz_friends, new_friends[i], NULL);
  	dictionary_t *friendz_friends = (dictionary_t *)dictionary_get(users, new_friends[i]);
  	if (friendz_friends != NULL)
  	{
  		dictionary_set(friendz_friends, user, NULL);
  	}
  	else
  	{
  		dictionary_t *new_user = make_dictionary(1,free);
  		dictionary_set(new_user, user, NULL);
  		dictionary_set(users, new_friends[i], new_user);
  	}
  }

  const char **friends_list= dictionary_keys(userz_friends);
  body = join_strings(friends_list, '\n');

  free(friends_list);

  len = strlen(body);

  /* send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  //free(body);
}

/*
 * serve_unfriend - unfriends some users and reports the friends of a user defined in the query
 */
static void serve_unfriend(int fd, dictionary_t *query)
{
  printf("serve unfriend\n");
  size_t len;
  char *body, *header, *user, *friends;

  user = (char *)dictionary_get(query, "user");
  friends = (char *)dictionary_get(query, "friends");
  if (!user || !friends)
  {
  	clienterror(fd, "?", "400", "Bad Request", "Please provide two valid arguments");
  	return;
  }

  dictionary_t *userz_friends = (dictionary_t *)dictionary_get(users, user);
  if (userz_friends == NULL)
  {
  	dictionary_t *new_user = make_dictionary(1,free);
  	dictionary_set(users, user, new_user);
  	userz_friends = (dictionary_t *)dictionary_get(users, user);
  }
  else
  {
  	char **new_friends = split_string(friends, '\n');
  	int i;
  	for(i = 0; new_friends[i] != NULL; i++)
  	{
  	  dictionary_remove(userz_friends, new_friends[i]);
  	  dictionary_t *friendz_friends = (dictionary_t *)dictionary_get(users, new_friends[i]);
  	  if (friendz_friends != NULL)
  	  {
  		dictionary_remove(friendz_friends, user);
  	  }
  	  else
  	  {
  		dictionary_t *new_user = make_dictionary(1,free);
  		dictionary_set(new_user, user, NULL);
  	  }
    }
  }

  const char **friends_list= dictionary_keys(userz_friends);
  body = join_strings(friends_list, '\n');

  free(friends_list);

  len = strlen(body);

  /* send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  //free(body);
}

/*
 * serve_introduce - introduces a user A to another user B and all of B's friends
 */
static void serve_introduce(int fd, dictionary_t *query)
{
  printf("serve introduce\n");
  size_t len;
  char *body, *header, *user, *host, *port, *request, response[MAXLINE];
  int client;
  rio_t rio;

  user = dictionary_get(query, "user");
  host = dictionary_get(query, "host");
  port = dictionary_get(query, "port");
  if (!user || !host || !port)
  {
  	clienterror(fd, "?", "400", "Bad Request", "Please provide a user");
  	return;
  }

  dictionary_t *friends = (dictionary_t *)dictionary_get(users, user);
  if (friends == NULL)
  {
  	dictionary_t *new_friends = make_dictionary(1,free);
  	dictionary_set(users, user, new_friends);
  	friends = (dictionary_t *)dictionary_get(users, user);
  }
  //request = append_strings(host,port,"/friends?user=",user);
  //struct addrinfo hints;
  //struct addrinfo *addrs;
  //memset(&hints, 0, sizeof(struct addrinfo));
  //hints.ai_family = AF_INET;
  //hints.ai_socktype = 0;
  //Getaddrinfo(host, port, &hints, &addrs);

  //s = Socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
  //Connect(s, addrs->ai_addr, addrs->ai_addrlen);
  //Freeaddrinfo(addrs);
  //struct sockaddr server_addr;
  //int addrlen = sizeof(sockaddr);
  request = "GET /friends/ HTTP/1.0\r\n\r\n";     // make the request header
  len = strlen(request);                          // 
  client = Open_clientfd(host,port);              // client = the connection file descriptor
  Rio_writen(client, request, len);               // write the request header to the client file descriptor

  Rio_readinitb(&rio, client);
  Rio_readnb(&rio, response, MAXLINE);
  

  len = strlen(body);

  /* send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  //free(body);
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
  size_t len;
  char *header, *body, *len_str;

  body = append_strings("<html><title>Friendlist Error</title>",
                        "<body bgcolor=""ffffff"">\r\n",
                        errnum, " ", shortmsg,
                        "<p>", longmsg, ": ", cause,
                        "<hr><em>Friendlist Server</em>\r\n",
                        NULL);
  len = strlen(body);

  /* Print the HTTP response */
  header = append_strings("HTTP/1.0 ", errnum, " ", shortmsg, "\r\n",
                          "Content-type: text/html; charset=utf-8\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n\r\n",
                          NULL);
  free(len_str);
  
  Rio_writen(fd, header, strlen(header));
  Rio_writen(fd, body, len);

  free(header);
  free(body);
}

static void print_stringdictionary(dictionary_t *d)
{
  int i, count;

  count = dictionary_count(d);
  for (i = 0; i < count; i++) {
    printf("%s=%s\n",
           dictionary_key(d, i),
           (const char *)dictionary_value(d, i));
  }
  printf("\n");
}
