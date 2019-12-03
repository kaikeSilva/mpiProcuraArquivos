#include <iostream>
#include <fstream>
#include <dirent.h>
#include <vector>
#include <iterator>
#include "md5.h"
#include <map>
#include <mpi.h>

using namespace std;

string binarios[10000];
int i=0;


//para usar esta função deve-se passar o caminho para o diretorio sem a barra final
//ex: nomeDir = pasta1/pasta2 (sem a barra final), nomeArque = teste
string calcularHash (string nomeArq, string nomeDir = "") {

    if(nomeDir != ""){
        string caminho = nomeDir+"/"+nomeArq;
    } else {
        string caminho = nomeArq;
    }

        // abrir o arquivo:
        std::streampos fileSize;
        string binario;
        std::ifstream file;
        file.open(caminho);

        if ( file.is_open() ) {

            // ver tamanho do arquivo:
            file.seekg(0, std::ios::end);
            fileSize = file.tellg();
            file.seekg(0, std::ios::beg);
            if ( fileSize <= 0 ) {
                cout<<"arquivo vazio"<<endl;
            }

            // ler dados binarios para vetor:
            std::vector<unsigned char> fileData(fileSize);
            file.read((char*) &fileData[0], fileSize);

            //transformar binario do arquivo em string
            for (int i=0;i<fileData.size();i++) {
                //cout<<fileData.at(i)<<endl;
                binario += fileData.at(i);
            }
            file.close();
        }
        else {

            cout<<"impossivel abrir"<<endl;
        }

        return binario;
}

void percorrerDir (string nomeDir) {

    //abrir diretorio
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir (nomeDir.c_str())) != NULL) {
      /* percorrer todos os arquivos dentro de um diretorio*/
      while ((ent = readdir (dir)) != NULL) {

        //se for arquivo guardar binario na matriz se diretorio entrar nele
          if(ent->d_type == DT_DIR ) {

              //verificar se é um diretório válido
              if(ent->d_name[0] != '.'){

                  //concatenar caminho

                  percorrerDir(nomeDir+ '/'+ent->d_name);
              }

          } else {

              //pegar o binario do arquivo e armazenar no vetor
              string binario = calcularHash(ent->d_name,nomeDir);

              if(i<10000){
                  binarios[i] = binario;
              } else {
                  throw "matriz cheia";
              }


          }
      }
      closedir (dir);
    } else {
      /* impossivel abrir diretorio */
      perror ("");

    }
}

int main(int argc, char **argv)
{
    int m=0;
    string diretorio;
    //variaveis para armazenar
    //world_rank = numero individual de processos
    //world_size = quantidade de processos que vao estar executando
    MPI_Init(&argc, &argv);//iniciar trecho paralelo
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    //quantidade de elementos que cada rank ira preencher
    int comprimento = i%(world_size);

    //map para armazenar hash dos binarios, estrutura armazena tipo chave: valor
    map<string, int> hashs;

    if (world_rank == 0)
    {
    //o primeiro processo vai preencher a matriz com os binarios dos arquivos

        //pegar o nome do diretorio e percorer suas pastas e subpastas
        cin>>diretorio;



        //percorrer os diretorios e subdiretorios dentro do diretorio passado
        try {
            percorrerDir(diretorio);

        } catch (exception e) {
            cout<<e.what()<<endl;
        }

        //MPI_barrier espera ate que todos os ranks tenham terminado as suas execuções
        //o codigo a partir daqui só é executado quando todos terminam
        MPI_Barrier(MPI_COMM_WORLD);

        map<string, int>::iterator it;

        //arquivo deve estar na mesma pasta do programa
        string nomeDoArquivo;
        cin>>nomeDoArquivo;
        string binarioBusca = calcularHash(nomeDoArquivo);

        //buscar a hash no map
        it = hashs.find(binarioBusca);

        if(it == hashs.end())
            cout << "arquivo não existe nas pastas procuradas" << endl ;
        else
            cout << "chave encontrada na relação chave, valor : "
              << it->first << "->" << it->second << endl;


    } else //os outros processos irão calcular as hash's
    {
        //popular a matriz de hash's na estrutura map, que armazena chave valor
        for (int k=0;k<=comprimento; k++) {
            hashs[MD5(binarios[(world_rank-1)+k]).hexdigest()] = i+k;
        }
        //ao sair do for o vetor de hashs estara pronto


        //mpi_barrier vai sinalizar quando todos os ranks terminarem
        MPI_Barrier(MPI_COMM_WORLD);
    }


    return 0;
}
