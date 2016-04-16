#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <jansson.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "parser.h"
#include "queue.h"
#include "protocol.h"
#include "worker.h"

pthread_t *thr = NULL;
unsigned int threads_ammount = 4;
struct worker_data* worker_data = NULL;
bool run = true;
unsigned int correct_shares = 0;
unsigned int incorrect_shares = 0;
// struct worker_data* thr_data;

// void sig_handler(int signo) {
//   if (signo == SIGINT) {
//     printf("Killing miners, signal received\n");
//     run = false;
//   }
// }

int main(int argc, char **argv) {
  /* Catch SIGINT */
  // if(signal(SIGINT, sig_handler) == SIG_ERR)
	 //  printf("\ncan't catch SIGINT\n");

  /* Parse arguments */
  struct arguments args;
  parse_arguments(argc, argv, &args);
  if(!args.host || !args.port || !args.username) {
  	print_usage();
  	return 0;
  }
  printf("Connecting with: \n");
  printf("Host: %s:%s \nUser: %s:%s \n", args.host, 
  	args.port, args.username, args.password);

  /* Ammount of threads available */
  long processors = sysconf(_SC_NPROCESSORS_ONLN);
  if(!args.cores || args.cores > processors) {
    threads_ammount = (unsigned int) processors;
  }
  else {
    threads_ammount = args.cores;
  }

  struct stratum_connection *connection = create_stratum_connection(args.host, args.port);
  if(connection == NULL) {
    printf("Error connecting to stratum server \n");
    return 1;
  }
  printf("Successfully connected \n");

  struct stratum_context *context = create_stratum_context();

  stratum_subscribe(connection);
  stratum_authorize(connection, args.username, args.password);

  char *message = NULL;
  size_t message_received_size = 0;
	while(run) {
    if(send_messages(connection) < 0) {
      printf("Error writing to socket exiting \n");
      run = false;
    }
    // printf("Getting messages \n");
    message_received_size = get_message(connection, &message);
    printf("Got message: \n %s \n", message);
		/* Nothing to read */
		if(message_received_size == 0) {
      printf("Server timed out \n");
      break;
    }
		/* Process message */
		json_error_t *decode_error = NULL;
		json_t *json_obj = json_loads(message, JSON_DECODE_ANY, decode_error);
		if(!json_obj) {
			printf("Error decoding JSON \n");
			printf("%s \n", decode_error->text);
			printf("Error found in: \n");
			printf("%s \n", decode_error->source);
		}
		json_t *id = json_object_get(json_obj, "id");
		/* If id is not null, it's a response */
		if(!json_is_null(id)) {
			json_int_t id_value = json_integer_value(id);
      json_t *error = json_object_get(json_obj, "error");
      json_t *result = json_object_get(json_obj, "result");
			switch(id_value) {
				case 1:
					if(context->state == SUBS_SENT) {
						printf("Recieved subscription \n");
						context->state = AUTH_SENT;
						stratum_load_subscription(context, json_obj);
					}
					else {
						printf("Error no deberia estar este mensaje \n");
						// Matar todo?
            break;
					}
					break;
				case 2:
					if(context->state == AUTH_SENT) {
						printf("Recieved authorization \n");
						context->state = AUTHORIZED;
						if(!json_is_true(result)) {
							printf("Error subscribing user \n");
              run = false;
              break;
						}
					}
					else {
						printf("Error no deberia estar este mensaje \n");
						// Matar todo?
            break;
					}
					break;

				default:
          if(json_is_null(error) && json_is_true(result)) {
            /* {"error": null, "id": 4, "result": true} */
            ++correct_shares;
            printf("Share aproved! \n Accepted shares: %d/%d \n", correct_shares, 
              correct_shares + incorrect_shares);
          }
          else {
            /* {"id": 10, "result": null, "error": (21, "Job not found", null)} */
            ++incorrect_shares;
            printf("Error submiting shares: %s \n Accepted shares: %d/%d \n", 
              json_string_value(json_array_get(error, 1)), correct_shares, 
              correct_shares + incorrect_shares);
          }
					break;

			}
		}
		else {
			// Recibo oferta de trabajo o cambiar dificultad, etc
			json_t *method = json_object_get(json_obj, "method");
			if(!strcmp(json_string_value(method), "mining.notify")) {
				printf("Server sent a notify \n");
				stratum_notify(context, json_obj);
        if(!thr) create_threads(&thr, &worker_data, connection, context, threads_ammount);
        if(context->job.clean) {
          context->job.clean = false;
          reset_threads(worker_data, threads_ammount);
        }
			}
			else if(!strcmp(json_string_value(method), "mining.set_difficulty")) {
        stratum_set_next_job_difficulty(context, json_obj);
        printf("Server sent new difficulty: %.1f \n", context->next_diff);
			}
      else if(!strcmp(json_string_value(method), "client.show_message")) {
        json_t *params = json_object_get(json_obj, "params");
        json_t *val = json_array_get(params, 0);
        printf("Message from the server \n %s \n", json_string_value(val));
      }
      else if(!strcmp(json_string_value(method), "client.get_version")) {
        stratum_client_version(connection, json_obj);
      }
      else if(!strcmp(json_string_value(method), "client.reconnect")) {
        printf("The server asked for a reconnection, exiting \n");
        run = false;
      }
		}
		json_decref(json_obj);
		free(message);
	}
  printf("Exiting \n");
  if(thr) kill_threads(thr, worker_data, threads_ammount);
  destruct_stratum_connection(connection);
  // printf("Destroyed connection \n");
  destruct_stratum_context(context);
  // printf("Destroyed context \n");
  return 0;
}
