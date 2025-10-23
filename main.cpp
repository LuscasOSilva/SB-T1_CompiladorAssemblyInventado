#include <bits/stdc++.h>

using namespace std;

int main(int argc, char *argv[]){
    int cod = 0;
    if(argc < 2){
        cerr << "é preciso um arquivo de entrada" << endl;
        return 1;
    }

    string nomeArquivo = argv[1];

    ifstream arquivo(nomeArquivo);

    if (arquivo.is_open()) {
            string palavra;
            while (arquivo >> palavra) {
                cout << palavra << " n " << cod << endl;
                cod++;
            }
            arquivo.close();
        } else {
            cerr << "Não foi possível abrir o arquivo: " << nomeArquivo << endl;
            return 1; // Indica erro
        }


    return 0;
}