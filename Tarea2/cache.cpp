#include"cache.h"
#include"math.h"
#include"cmath"
#include <bitset>

//Metodo Contructor Cache
// Parametros: int tamano del cache, int tamano del bloque, int associatividad(numero de vias)
// Crea un elemento de la clase cache, inicializa todos sus atributos y genera la matriz de bloques de cache segun los parametros dados
Cache::Cache(int cache_size_in, int b_size, int Assoc){
  int cache_1000 = cache_size_in*1000;
  int cache_size = 2;
  while (cache_1000 > cache_size) {
    cache_size=cache_size*2;
  }
  // En este momento ya se tiene el tamaño real de cache.

  int block_count = cache_size/b_size/Assoc; // Se consigue el numero de bloques por via del cache

  //  Calculo de bits de index
  float block_count_float = (float)block_count; // Se convierte a float el numero de bloques por via
  this->index_bit_count = (int)(log2(block_count_float)); // Se le asigna a index_bit_count la cantidad de bits del index
  // Calculo de bits de offset
  float b_size_float = (float)b_size;
  this->offset_bit_count = (int)(log2(b_size_float)); // Se calculan la cantidad de bits de offset

  this->Assoc=Assoc; // Se le asigna a Assoc la cantidad de vias
  //Bits de Tag
  this->tag_bit_count = 32-this->offset_bit_count-this->index_bit_count;

  //Se crea la mascara de comparacion de index:
  for (int i = 0; i < this->index_bit_count; i++) {
    this->mask_index = this->mask_index << 1;
    this->mask_index = this->mask_index + 1;
  }
  this->mask_index = this->mask_index << this->offset_bit_count;
//--------------------------------------------------------------------------
  //Se crea la mascara de comparacion de tags
  for (int i = 0; i < this->tag_bit_count; i++) {
    this->mask_tag = this->mask_tag << 1;
    this->mask_tag = this->mask_tag + 1;
  }
  this->mask_tag = this->mask_tag << (this->offset_bit_count+this->index_bit_count);
//----------------------------------------------------------------------------
  //Se crea la matriz con bloques del chache
  this->cache_head= new Block*[Assoc]; // Se le asiga a cache_head un vector de punteros a bloque del tamano de la vias
  for (int i = 0; i < Assoc; i++) {
    this->cache_head[i]= new Block[block_count]; // a cada uno de esos punteros se le asigna un vector del tamano del numero de bloques por via

  }
  // Se llena el los bloques con su index respectivo
  for (int j = 0; j < block_count; j++) {
    for (int i = 0; i < Assoc; i++) {
      this->cache_head[i][j].Index=j;
    }/* code */
  }
}
//Destructor de la clase
Cache::~Cache(void){

}
// Metodo check_addr
// Parametos: int addr, son las direcciones de memoria del trace, int LS es in entero que indica si la direccion es un load o un store, 0 si es Load, 1 si es Store
// El metodo se encarga de leer las direcciones que vienen en addrs, y revisar si esos tag estan en el cache, si estan es un hit, sino se verifica si el bloque esta vacio o lleno, dependiendo del resultado de procede a agregar el bloque o a victimizar uno
void Cache::check_addr(int addr, int LS){

  int addr_index = (addr & this->mask_index) >> this->offset_bit_count ; // Se consige el index de la direccion comparando con la mascara de index
  int addr_tag = (addr & this->mask_tag) >> (this->offset_bit_count+this->index_bit_count); // se consigue el tag de la direccion comparando con la mascara de tag
  int assoc_counter=0; // Se inicializa el contador que va a recorrer las vias para verificar si para el index de la dirrecion se puede guardar

  while (assoc_counter < this->Assoc) { // bucle que recorre las vias

    if (this->cache_head[assoc_counter][addr_index].empty==1) { // se revisa si para esa posicion en la matriz de bloques esta vacia,

     this->add_block(assoc_counter, addr_index, addr_tag, LS); // si lo esta re agrega al bloque al cache
     if (LS==0) { // Si LS  es un 0 se suma 1 al contador de misses de load
       load_misses++;
     } else {
       store_misses++; // Si es un 1 se incrementa el contador de misses de store
     }
     break; // se sale del bucle
    } else { // si el bloque no esta vacio
      if (this->cache_head[assoc_counter][addr_index].Tag == addr_tag ) { // se verifica que el tag sea igual
        if (LS==0) { // si el tag es igual es un 1, se suma 1 a los contadores de hits dependiendo de LS
          load_hits++;
        } else {
          store_hits++;
          this->cache_head[assoc_counter][addr_index].dirty_bit=1; // Si es un hit de Store, el dirty bit del cache se pone en 1
        }
        this->cache_head[assoc_counter][addr_index].RRPV = 0;  // En hit se le asigna un RRPV de 0 al bloque
        break; // se sale del bucle
      } else{ // si no esta vacio y el tag es diferente se pasa a la siguiente via
        assoc_counter++;
      }
    }
    if (assoc_counter == this->Assoc) { // si el contador llega a ser mismo que el numero de vias se recorrienron todas la vias, por lo que estaban todas ocupabadas y no se dieron hits
      this->victim(addr_index, addr_tag, LS); // Se llama al metodo victimizar para reemplazar un bloque dependiendo de la politica de reemplazo
      if (LS==0) { // Se suman 1 a los contadores misses dependiendo de la instruccion
        load_misses++;
      } else {
        store_misses++;
      }
    }
  }
}

//Metodo add_block
//Parametros: int via, donde es la via donde se va a agregar el bloque
//            int index, index del bloque que se va agregar
//            int tag, tag del bloque que contiene la direccion
//            int db, el dirty bit del bloque a insertar dependiendo de la intruccion
// Agrega un bloque al cache ubicando su posicion con la via y el index, y le asigna sus atributos respectivos al bloque

void Cache::add_block(int via, int index, int tag, int db){ // se usa la via y el indez para ubicar el bloque
  this->cache_head[via][index].Tag=tag;
  this->cache_head[via][index].empty=0; // el bloque se inserta esta ocupado
  this->cache_head[via][index].RRPV=2; // siempre se insertan nuevos bloques con RRPV de 2
  this->cache_head[via][index].dirty_bit=db; // Se le asigna el tipo de instrucicon del bloque

}
//Metodo victim
//Parametros: int index, recibe el index del bloque de la dirrecion que se quiere agregar
//            int tag, recibe el tag del bloque de la direccion que se quiere agregar
//            int db, recibe el tipo de instruccion del bloque que se va a agregar
//Se encarga de revisar las vias para un index dado, y revisa sus RRPV, si es 3 ese bloque es el que se va a reemplazar, sino hay una via con un RRPV de 3 se le suma 1 a todos los RRPV de todas las vias para ese index
void Cache::victim(int index, int tag, int db){
  int assoc_counter=0; // se inicializa el contador de vias
  while (true) { // se inciia un bucle infinito
    if (assoc_counter==this->Assoc) { // si se recorriendo todas las vias y no de encontro uno con RRPV de 3
      increase_RRPV(index); // se llama al metodo de increase_RRPV
      assoc_counter=0; // se reinicia el contador para verificador de nuevo
    }
    if (this->cache_head[assoc_counter][index].RRPV == 3) { // Si la para la via del contador si tiene un RRPV de 3
      if (this->cache_head[assoc_counter][index].dirty_bit ==1) { // Se revisa si el db del bloque a reemplazar es 1
        dirty_evictions ++; // si lo es se incrementa el contador de dirty_evictions en 1
      }
      this->add_block(assoc_counter, index, tag, db); // Se agrega el nuevo bloque en esa posicion
      break; // se sale del ciclo
    } else {
      assoc_counter++; // si no se cumplen las codiciones se mantiene aumentando el contador
    }
  }
}
//Metodo increase_RRPV
//Parametos: int index, index para el cual se van a recorrer todas las vias de ese index
// El metodo se encarga de recorrer todas las vias para el index de entrada e incrementar el RRPV de cada bloque en 1 si este es dirente de 3
void Cache::increase_RRPV(int index){
  for ( int i = 0; i < this->Assoc; i++) {
    if (this->cache_head[i][index].RRPV != 3) {
      this->cache_head[i][index].RRPV ++;
    }
  }
}
