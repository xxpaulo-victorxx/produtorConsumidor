#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <thread> 
#include <mutex> 
#include <condition_variable>
#include <chrono>
#include <atomic>


using namespace std;

vector<int> sharedMemory; // vetor com N posicoes (zeros)
vector<int> consumerMemory;

// variáveis do semáforo
mutex s_mutex; // semáforo mutex 
condition_variable s_full;  // semáforo que indica memória cheia
condition_variable s_empty;  // semáforo que indica memória vazia

int numbersToProcess;
int spaces;
int elements = 0;
atomic<int> p_working(0);
int produced = 0;
int consumed = 0;

int getRandomNumber(int p){
    srand(p+time(NULL));
    int number = rand() % 10000000 + 1;
    return number;
}

bool primoTeste(int number){
    int i;
    for (i=2; i<number; i++){
        if (number % i == 0){
            return false;
        }
    }
    return true;
}

int findEmptyPosition(vector<int> myVector){
	// retorna -1 caso o vetor esteja cheio
	int targetPosition = -1;
	int i = 0;
	while(i < myVector.size()){
		if(myVector[i] == 0){
			targetPosition = i;
			break;
		}
		i++;
	}
	return targetPosition;
}

int findProductPosition(vector<int> myVector){
	// retorna -1 caso o vetor esteja vazio
	int targetPosition = -1;
	int i = 0;
	while(i < myVector.size()){
		if(myVector[i] != 0){
			targetPosition = i;
			break;
		}
		i++;
	}
	return targetPosition;
}

void produce(int p_id){
	unique_lock<mutex> lock(s_mutex);
	int positionToStore = findEmptyPosition(sharedMemory); // -1 caso o vetor esteja cheio
	if(s_full.wait_for(lock,chrono::milliseconds(200),[] {return elements != spaces ;}) && positionToStore  != -1){
		int product = getRandomNumber(p_id);
		sharedMemory[positionToStore] = product; // colocando o número na memória
		elements += 1;
		produced +=1;
		s_empty.notify_all();
	}
}

void consume(){
	unique_lock<mutex> lock(s_mutex);
	int positionToGet = findProductPosition(sharedMemory); // -1 caso o vetor esteja vazio
	if(s_empty.wait_for(lock,chrono::milliseconds(200),[] {return elements > 0;}) && positionToGet  != -1){
		cout << sharedMemory[positionToGet]<< " is prime? The answer is: "<< primoTeste(sharedMemory[positionToGet])<< endl;
		consumerMemory.push_back(sharedMemory[positionToGet]); // Salvando o número da memória compartilhada na memória local
		sharedMemory[positionToGet] = 0; //  limpando a posicao da memória após a leitura
		elements -= 1;
		consumed +=1;
		s_full.notify_all();
	}
}


void consumer(){
	while(p_working == 0){
		this_thread::yield();
	}
	while(consumed < numbersToProcess){
		consume();
	}
}

void producer(){
	p_working += 1;
	while(produced < numbersToProcess){
		produce(produced);
	}
	p_working -= 1;
}

int main(int argc, char* argv[]) {
	
	spaces = atoi(argv[1]); // Primeiro argumento é N (número total de espacos na memória)
	
	int numberProducerThreads = atoi(argv[2]);  // Segundo argumento é o número correspondente a quantidade de produtores (threads)
	int numberConsumerThreads = atoi(argv[3]);  // Terceiro argumento é o número correspondente a quantidade de consumidores (threads)
	

	// Medindo o tempo decorrido 
	chrono::time_point<std::chrono::system_clock> start, end;
	start = chrono::system_clock::now();


	int numberOfThreads = numberConsumerThreads+numberProducerThreads;
	numbersToProcess = 1000; // total de números que o Consumidor irá processar
	thread producerConsumerThreads[numberOfThreads];
	// Preenchendo os espacos de memória com 0 (zeros)
	for (int i = 0; i < spaces; ++i){
		sharedMemory.push_back(0);
	}
	// Lancando as  Threads
	for(int i = 0; i < numberProducerThreads;i++){
		producerConsumerThreads[i] = thread(producer);
	}
	for(int i = numberProducerThreads; i < numberOfThreads;i++){
		producerConsumerThreads[i] = thread(consumer);
	}
	// Reunindo as Threads
	for (int i = 0; i < numberOfThreads;i++){ 
		producerConsumerThreads[i].join();
	}

	end = chrono::system_clock::now();
	long elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds> (end-start).count();
	cout << endl << "Tempo de Processamento = " << elapsed_seconds << "mS (milisegundos)" << endl << endl ;
	return 0;
}