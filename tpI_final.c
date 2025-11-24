#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

struct memory {
  char *response;
  size_t size;
};

static size_t cb(char *data, size_t size, size_t nmemb, void *clientp) //debe retornar la cantidad de bytes procesados
{
  size_t realsize = nmemb; //size = 1, porque es un char
  struct memory *mem = clientp;

  char *ptr = realloc(mem->response, mem->size + realsize + 1); //se le suma 1 por el 0 en el final 
  if(!ptr)  //variable auxiliar, para que si devuelve null no perder el buffer original
    return 0;  /* out of memory */

  mem->response = ptr; //tambien ayuda a asegurarnos de que realloc no redireccione a cualquier lado
  memcpy(&(mem->response[mem->size]), data, realsize); //funcion de string.h, copia bytes (a donde quiere copiar, desde donde, cuanto)
  mem->size += realsize; //actualiza el size
  mem->response[mem->size] = 0; // para que sea una string valido

  return realsize;
}

long int extraerUpdateId (CURL *curl, struct memory chunk){
    
    char *p = strstr(chunk.response, "\"update_id\"");

    if(p){

    char cadena_update_id[50];
    int i = 0;
	
    while(*p < '0' || *p > '9'){
    	p++;
    }
    while(*p >= '0' && *p <= '9'){
        cadena_update_id[i++] = *(p++);
    }

    cadena_update_id[i] = '\0';

    long int update_id = atol(cadena_update_id);
    return update_id;

     }

    return 0;
}

long int extraerChatId(CURL *curl, struct memory chunk){
    
    char *p = strstr(chunk.response, "\"chat\"");
    
    if(p){

       p = strstr(p, "\"id\"");
       if(p){

        char cadena_chat_id[50];
        int i = 0;

        while((*p < '0' || *p > '9') && (*p != '-')){
          p++;
        }
        while((*p >= '0' && *p <= '9') ||( *p == '-')){
          cadena_chat_id[i++] = *(p++);
        }
     
        cadena_chat_id[i] = '\0';


        long int chat_id = atol(cadena_chat_id);
	
	return chat_id;
       }
    }
    return 0;
}

void extraerNombre(char *nombre, struct memory chunk){
    char *p = strstr(chunk.response, "\"from\"");
    int i = 0;

      if(p){
        p = strstr(p, "\"first_name\"");
        if(p){
          p += strlen ("\"first_name\"");
          p = strchr(p, '"');
          p++;

          while(*p != '"'){
                nombre[i++] = *(p++);
          }

          nombre[i] = '\0';
        }
      }
}


long int extraerHora(struct memory chunk){
    char *p = strstr(chunk.response, "\"date\"");
    if(p){
        char cadena_hora[30];
        int i = 0;
        while(*p < '0' || *p > '9'){
            p++;
        }
        while(*p >= '0' && *p <= '9'){
            cadena_hora[i++] = *(p++);
        }

	cadena_hora[i] = '\0';

        return atol(cadena_hora);
    }
    return 0;
}

void registrarHora(long int timestamp, const char *nombre, const char *mensaje){
    FILE *fp = fopen("registro.txt", "a");
    if(fp){
        fprintf(fp, "%ld | %s: %s\n", timestamp,  nombre, mensaje);
        fclose(fp);
    } else {
        printf("Error: No se pudo abrir el archivo\n");
    }
}

void responderMensaje(CURL *curl, struct memory chunk, long int chat_id, CURLcode res, char *token, long int hora){
    char *p = strstr(chunk.response, "\"chat\"");
    char mensaje[500];
    char nombre[30];
    int i = 0;

      if(p){
        p = strstr(p, "\"text\"");
        if(p){
	  p += strlen ("\"text\"");
	  p = strchr(p, '"');
	  p++;

	  while(*p != '"'){
	  	mensaje[i++] = *(p++);
	  }
         
          mensaje[i] = '\0';

         extraerNombre(nombre, chunk);
         registrarHora(hora, nombre, mensaje);

         for(int j = 0 ; j < strlen(mensaje) ; j++){
	   if(mensaje[j] >= 'A' && mensaje[j] <= 'Z'){
	     mensaje[j] += 32;
	   }
	 }

	 char nueva_url[500];
	 char respuesta[200];

	  if (strstr(mensaje, "hola")) {
	     snprintf(respuesta, sizeof(respuesta), "Hola %s, como estas?", nombre);
	     snprintf(nueva_url, sizeof(nueva_url), "https://api.telegram.org/bot%s/sendMessage?chat_id=%ld&text=Hola%%20%s,%%20como%%20estas%%3F", token, chat_id, nombre);
	     curl_easy_setopt(curl, CURLOPT_URL, nueva_url);
	     res = curl_easy_perform(curl);
	     registrarHora(hora, "BOT", respuesta);

	  } else if (strstr(mensaje, "chau")) {
	      snprintf(respuesta, sizeof(respuesta), "Chau %s, que andes bien", nombre);
	      snprintf(nueva_url, sizeof(nueva_url), "https://api.telegram.org/bot%s/sendMessage?chat_id=%ld&text=Chau%%20%s,%%20que%%20andes%%20bien", token, chat_id, nombre);
              curl_easy_setopt(curl, CURLOPT_URL, nueva_url);
	      res = curl_easy_perform(curl);
	      registrarHora(hora, "BOT", respuesta);
   	    }
        }
      }
}

void actualizarId(CURL *curl, long int update_id, char *token){
    char nueva_url[500];
    snprintf(nueva_url, sizeof(nueva_url), "https://api.telegram.org/bot%s/getUpdates?offset=%ld", token, update_id+1);
    curl_easy_setopt(curl, CURLOPT_URL, nueva_url);
}

void extraerToken(char *token){
    FILE *tk = fopen("token.txt", "r");
    if(tk){
      fscanf(tk, "%s", token);
      fclose(tk);
    }else{
      printf("No se pudo obtener el token");
      exit(1);
    }
}


void limpiarChunk(struct memory *chunk){
    free(chunk->response);
    chunk->response = 0;
    chunk->size = 0;
}

int main(void)
{
  char token[100];
  extraerToken(token);
  char api_url[500];
  snprintf(api_url, sizeof(api_url), "https://api.telegram.org/bot%s/getUpdates", token);
  long int update_id = 0;


  CURLcode res;  //enum. Lo que retorna perform
  CURL *curl = curl_easy_init(); //handle. objeto para configurar la peticion
  struct memory chunk = {0};
  


  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, api_url); //configurar el dominio
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb); //configurar el callback al que llamara libcurl cada vez que se reciban datos
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk); //configurar puntero que el cb recibe con clientp

   
    while(1){
    res = curl_easy_perform(curl); //ejecuta todo, no retorna hasta que la peticion termina (en exito o error)
    if (res != 0)
      printf("Error Código: %d\n", res);

    printf("%s\n", chunk.response); 

    update_id = extraerUpdateId(curl, chunk);	

    long int chat_id = extraerChatId(curl, chunk);

    long int hora = extraerHora(chunk);

    responderMensaje(curl, chunk, chat_id, res, token, hora);

    actualizarId(curl, update_id, token);

    limpiarChunk(&chunk);

    sleep(2);
    }

    curl_easy_cleanup(curl); //libera el handle
  }
}
