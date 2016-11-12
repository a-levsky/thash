/* Program required additional libraries to compile properly:
   gcc thash.c -std=c99 -lssl -lcrypto -lpthread -o thash */
#define _GNU_SOURCE
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <sys/resource.h>
#include <linux/limits.h>

#define TRUE 1
#define FALSE 0
#define FNAME_SIZE 512 //Maximum size of file name
#define DATA_SIZE 8096 //Maximum size of file contents to be read by hashing algorithms

char * a_values[4];

FILE * f_file;
FILE * e_file;
FILE * o_file;

typedef char file_name[FNAME_SIZE]; //create new type to represent a file name

typedef unsigned char md5_hash[MD5_DIGEST_LENGTH]; //create new type to represent md5 hashes
typedef unsigned char sha1_hash[SHA_DIGEST_LENGTH]; //create new type to represent sha1 hashes
typedef unsigned char sha256_hash[SHA256_DIGEST_LENGTH]; //create new type to represent sha256 hashes
typedef unsigned char sha512_hash[SHA512_DIGEST_LENGTH]; //create new type to represent sha512 hashes

int threads = 2; //default number of threads, can be specified with -t option
int file_total = 0; //counter for the total amount of files detected
int hashes_conducted = 0; //counter for hashes conducted (to avoid seg faults)

int algorithms = 0; //counter for the number of algorithms passed
int total_hashes = 0; //total amount of hashes to be performed
int md5_flag = FALSE;
int sha1_flag = FALSE;
int sha256_flag = FALSE;
int sha512_flag = FALSE;

char *token;

pthread_mutex_t lock; //mutex lock for threading
pthread_t thread_id;

struct Node{ //data structure for creating queue as a linked list
  file_name data;
  struct Node* next;
};

struct Node * front = NULL;
struct Node * rear = NULL;

void * Hash();
void Enqueue(file_name file_name_data);
void Dequeue();

int main(int argc, char **argv){

  int opt;
  int i = 0;
  while ((opt = getopt(argc, argv, "a:f:e:o:t:")) != -1){
    switch(opt){
      case 'a':
        token = strtok(optarg, ",");

        while(token != NULL){
          if(i > 3){
	    fprintf(stdout, "ERROR: Too many hashing algorithms passed, use up to 4 of md5,sha1,sha256,sha512\n");
	    return 1;
	  }

	  if(strcmp(token, "md5") != 0 && 
	     strcmp(token, "sha1") != 0 && 
	     strcmp(token, "sha256") != 0 && 
	     strcmp(token, "sha512") != 0)
	  {
	     fprintf(stdout, "ERROR: Invalid hashing algorithm '%s' passed, use up to 4 of md5,sha1,sha256,sha512\n", token);
	     return 1;
	  }

	  else if(strcmp(token, "md5") == 0){ 
            md5_flag = TRUE;
	    algorithms++; 
 	  }
	  else if(strcmp(token, "sha1") == 0){ 
	    sha1_flag = TRUE; 
	    algorithms++;
          }
	  else if(strcmp(token, "sha256") == 0){ 
	    sha256_flag = TRUE; 
	    algorithms++;
	  }
	  else if(strcmp(token, "sha512") == 0){ 
	    sha512_flag = TRUE; 
	    algorithms++;
	  }

          a_values[i] = malloc(strlen(token) + 1);
	  strcpy(a_values[i], token);
	  token = strtok(NULL, ",");
	  i++;
        }

	break;
      case 'f':
        f_file = fopen(optarg, "r");
        if(f_file == NULL){
	  fprintf(stdout, "ERROR: File list file '%s' could not be opened\n", optarg);
	  perror("Error");
          return 1;
	}

	break;
      case 'e':
	e_file = fopen(optarg, "w");
	if(e_file == NULL){
	  fprintf(stdout, "ERROR: Error output file '%s' could not be opened\n", optarg);
          return 1;
	}
	break;	
      case 'o':
	o_file = fopen(optarg, "w");
	if(o_file == NULL){
	  fprintf(stdout, "ERROR: Result output file '%s' could not be opened\n", optarg);
          return 1;
	}
	break;
      case 't':
	threads = atoi(optarg);
	break;
      case '?':
	fprintf(stdout, "usage: thash -a ALGORITHMS [-f FILE] [-e FILE] [-o FILE] [-t NUM] FILE ...\n");
	return 1;
      default:
        fprintf(stdout, "usage: thash -a ALGORITHMS [-f FILE] [-e FILE] [-o FILE] [-t NUM] FILE ...\n");
	return 1;
     }
  }

  /* Check file name arguments and produce a list of file names */
  if (!f_file && optind >= argc) { //user passes no file list or command line file arguments
    fprintf(stdout, "ERROR: No file list or command line files specified\n");
    if(e_file != NULL){ fprintf(e_file, "ERROR: No file list or command line files specified\n"); }  
  }
  else if (f_file != NULL && optind < argc) { //user passes both file list and command line file arguments
    fprintf(stdout, "ERROR: Either file list or command line files must be specified, not both\n");
    if(e_file != NULL){ fprintf(e_file, "ERROR: Either file list or command line files must be specified, not both\n"); }  
  }
  else if (f_file == NULL && optind < argc) { //user passes command line file arguments
    int i = 0;
    file_total = argc - optind;
    //hash_file_names = malloc(sizeof(file_name) * (file_total)); //allocate memory to hold the file names passed as command line arguments
    while (optind < argc){
      Enqueue(argv[optind]);
      //strcpy(hash_file_names[i], argv[optind]);
      optind++;
      i++;
    }

  }
  else if (f_file != NULL && optind >= argc) { //user passes file list argument
    char new_line = fgetc(f_file);
    char ch = fgetc(f_file);

    while((ch = fgetc(f_file)) != EOF){ //determine number of files listed in file list
      if(ch == '\n'){
        file_total++;
      }
    }

    rewind(f_file); //rewind back to beginning of file

    //hash_file_names = malloc(sizeof(file_name) * (file_total)); //allocate memory to hold the file names contained in file list
    int i = 0;
    file_name file_line;

    while(fgets(file_line, sizeof(file_line), f_file) != NULL){ //read from file list
      int name_length = strlen(file_line);

      if(name_length > 0 && file_line[name_length - 1] == '\n'){ //replace new line character and add file name to list of file names
        file_line[name_length - 1] = '\0';
	Enqueue(file_line);
        //strcpy(hash_file_names[i], file_line);
      }
      i++;
    }
    fclose(f_file); //close file list
  }
  total_hashes = algorithms * file_total;

  for(int i = 0; i < threads; i++){ //create threads
    pthread_create(&thread_id, NULL, Hash, NULL);
  }

  pthread_join(thread_id, NULL);

  if(e_file != NULL){ fclose(e_file); };
  if(o_file != NULL){ fclose(o_file); };
  
return 0;

}
/* method for enqueuing file name to queue */
void Enqueue(file_name file_name_data){
  char absolute_path[PATH_MAX+1];
  struct Node * new_node = (struct Node*)malloc(sizeof(struct Node));
  
  strcpy(new_node->data, realpath(file_name_data, absolute_path));//convert all file paths to absolute paths

  new_node->next = NULL;

  if(front == NULL && rear == NULL){
    front = new_node;
    rear = new_node;
    return;
  }

  rear->next = new_node;
  rear = new_node;
}

/* method for dequeuing from the front and pushing the queue forward */
void Dequeue(){
  struct Node * current_node = front;

  if(front == rear){
    front = NULL;
    rear = NULL;
  }

  else{
    front = front -> next;
  }
  free(current_node);
}

/* perform hashing algorithms */
void * Hash(){

  int bytes;
  unsigned char data[8096];

  for(int i = 0; i < file_total; i++){
    pthread_mutex_lock(&lock);//lock resources

    file_name file_name_data;
    strcpy(file_name_data, front->data);
    Dequeue();//remove first value of queue after the file name has been read

    FILE * file = fopen(file_name_data, "r");
    
    printf("\n%s", file_name_data);
    if(o_file != NULL){ fprintf(o_file, "%s", file_name_data); }

    if(file == NULL){
      fprintf(stdout, "ERROR: File '%s' could not be opened\n", file_name_data);
      perror("Error");
      if(e_file != NULL){ 
	fprintf(e_file, "ERROR: File '%s' could not be opened\n", file_name_data); 
        exit(1);
      }
    }

    for(int i = 0; i < algorithms; i++){//loop through algorithms to ensure that algorithm output matches position of passed algorithm
      if(strcmp(a_values[i], "md5") == 0){//Perform md5 hashing
        MD5_CTX md5_ctx;
        md5_hash md5;
        MD5_Init(&md5_ctx);

        while((bytes = fread(data, 1, DATA_SIZE, file)) != 0){//read from file and continue updating the hash
          MD5_Update(&md5_ctx, data, bytes);
        }

        MD5_Final(md5, &md5_ctx);
        printf(",");
        if(o_file != NULL){ fprintf(o_file, ","); }

        for(int i = 0; i < MD5_DIGEST_LENGTH; i++){//print hash one character at a time
          printf("%02x", md5[i]);
	  if(o_file != NULL){ fprintf(o_file, "%02x", md5[i]); }
        }
        rewind(file);
	hashes_conducted++;
      }

      if(strcmp(a_values[i], "sha1") == 0){//Perform sha1 hashing
        SHA_CTX sha1_ctx;
        sha1_hash sha1;
        SHA_Init(&sha1_ctx);

        while((bytes = fread(data, 1, DATA_SIZE, file)) != 0){
          SHA_Update(&sha1_ctx, data, bytes);
        }

        SHA_Final(sha1, &sha1_ctx);
        printf(",");
        if(o_file != NULL){ fprintf(o_file, ","); }

        for(int i = 0; i < SHA_DIGEST_LENGTH; i++){
          printf("%02x", sha1[i]);
	  if(o_file != NULL){ fprintf(o_file, "%02x", sha1[i]); }
        }
        rewind(file);
	hashes_conducted++;
      }

      if(strcmp(a_values[i], "sha256") == 0){//Perform sha256 hashing
        SHA256_CTX sha256_ctx;
        sha256_hash sha256;
        SHA256_Init(&sha256_ctx);

        while((bytes = fread(data, 1, DATA_SIZE, file)) != 0){
          SHA256_Update(&sha256_ctx, data, bytes);
        }

        SHA256_Final(sha256, &sha256_ctx);
        printf(",");
        if(o_file != NULL){ fprintf(o_file, ","); }

        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
          printf("%02x", sha256[i]);
          if(o_file != NULL){ fprintf(o_file, "%02x", sha256[i]); }
        }
        rewind(file);
        hashes_conducted++;
      }

      if(strcmp(a_values[i], "sha512") == 0){//Perform sha512 hashing
        SHA512_CTX sha512_ctx;
        sha512_hash sha512;
        SHA512_Init(&sha512_ctx);

        while((bytes = fread(data, 1, DATA_SIZE, file)) != 0){
          SHA512_Update(&sha512_ctx, data, bytes);
        }

        SHA512_Final(sha512, &sha512_ctx);
        printf(",");
        if(o_file != NULL){ fprintf(o_file, ","); }

        for(int i = 0; i < SHA512_DIGEST_LENGTH; i++){
          printf("%02x", sha512[i]);
          if(o_file != NULL){ fprintf(o_file, "%02x", sha512[i]); }
        }
        rewind(file);
        hashes_conducted++;
      }
    }
    printf("\n\n");
    if(o_file != NULL){ fprintf(o_file, "\n"); }
    fclose(file); //close file pointer after file is processed
    pthread_mutex_unlock(&lock);//unlock resources
  }
}
