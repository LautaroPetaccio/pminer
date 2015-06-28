#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <jansson.h>
#include "parser.h"
#include "queue.h"
#include "protocol.h"

void sig_handler(int signo) {
  if (signo == SIGINT) {
  	// Kill other proccesses
    printf("received SIGINT\n");
   }
}

int main(int argc, char **argv) {
  /* Catch SIGINT */
  if(signal(SIGINT, sig_handler) == SIG_ERR)
	  printf("\ncan't catch SIGINT\n");

  /* Parse arguments */
  struct arguments args;
  parse_arguments(argc, argv, &args);
  if(!args.host || !args.port || !args.username) {
  	print_usage();
  	return 0;
  }
  printf("Connecting with: \n");
  printf("Host: %s:%s \n User: %s:%s \n", args.host, 
  	args.port, args.username, args.password);

  int socket = sock(args.host, args.port);
  printf("Successfully connected \n");
  /* Stratum work */
  struct work *work = create_work();
  // Creates stratum connection structure
  struct stratum_connection *connection = create_stratum_connection(socket);
  stratum_subscribe(connection);
  stratum_authorize(connection, args.username, args.password);
  printf("Antes de crear queue \n");
  connection_buffer *recv_queue = queue_create(12288);
  printf("Queue creada \n");
  char *message;
  size_t queue_received_size;
  printf("Starting main loop \n");
  int j = 0;
  while(j< 100) {
  	// Recieves data and places it in the queue
  	recv_data(connection->socket, recv_queue, queue_free_size(recv_queue));
  	// Proccess all the messages in the queue
  	j++;
  	while(1) {
  		queue_received_size = queue_pop(recv_queue, &message, '\n');
  		if(j == 100) {
  			printf("Printed queue \n");
  			print_queue_buffer(recv_queue);
  		} 	
  		// Nothing to read
  		if(queue_received_size == 0) break;
  		message[queue_received_size - 1] = '\0';
  		printf("Mensaje recibido: \n %s \n", message);
  		printf("Recieved message \n");
  		// Process message
  		json_error_t *decode_error = NULL;
  		json_t *json_obj = json_loads(message, JSON_DECODE_ANY, decode_error);
  		if(!json_obj) {
  			printf("Error decoding JSON subscription \n");
  			printf("%s \n", decode_error->text);
  			printf("Error found in: \n");
  			printf("%s \n", decode_error->source);
  		}
  		json_t *id = json_object_get(json_obj, "id");
  		// If id is not null, it's a response
  		if(!json_is_null(id)) {
  			json_int_t id_value = json_integer_value(id);
  			json_t *error = json_object_get(json_obj, "error");
  			if(!json_is_null(error)) {
  				printf("Error in response with id: %lld \n", id_value);
  			}
  			switch(id_value) {
  				case 1:
  					if(connection->state == SUBS_SENT) {
  						printf("Recieved subscription \n");
  						connection->state = AUTH_SENT;
  						stratum_load_subscription(connection, json_obj);
  					}
  					else {
  						printf("Error no deberia estar este mensaje \n");
  						// Matar todo?
  					}
  					break;
  				case 2:
  					if(connection->state == AUTH_SENT) {
  						printf("Recieved authorization \n");
  						connection->state = AUTHORIZED;
  						json_t *result = json_object_get(json_obj, "result");
  						if(!json_is_true(result)) {
  							printf("Error subscribing user \n");
  							// Matar todo?
  						}
  					}
  					else {
  						printf("Error no deberia estar este mensaje \n");
  						// Matar todo?
  					}
  					break;
  				default:
  					printf("Seguramente es un mensaje de aceptado de shares \n");
  					break;
  				//Tiene que ser un mensaje de aceptado de submit shares

  			}
  		}
  		else {
  			// Recibo oferta de trabajo o cambiar dificultad, etc
  			json_t *method = json_object_get(json_obj, "method");
  			if(!strcmp(json_string_value(method), "mining.notify")) {
  				printf("Recibi un notify \n");
  				stratum_notify(connection, json_obj);
  				create_new_job(connection, work);
  			}
  			else if(!strcmp(json_string_value(method), "mining.set_difficulty")) {
  				printf("Recibi un cambio de dificultad \n");
  			}
  		}
  		json_decref(json_obj);
  		free(message);
  	}
  }
  destruct_work(work);
  destruct_stratum_connection(connection);
  queue_free(recv_queue);
  close(connection->socket);
  return 0;
}