#include <bits/stdc++.h>

using namespace std;

// --- ESTRUTURAS DE DADOS PRINCIPAIS ---

// Tabela de Opcodes (Instrução -> {Opcode, Tamanho em palavras})
map<string, pair<int, int>> TabelaOpcodes;

// Tabela de Símbolos (Rótulo -> Endereço)
map<string, int> TabelaSimbolos;

// Guardar uma pendência
struct Pendencia {
    int enderecoNoCodigo; // Onde o '0' está (o índice no vetor CodigoObjeto)
    int deslocamento;     // O valor a somar (ex: o '1' em 'NUM+1')
};

// A nova Lista de Pendências
map<string, vector<Pendencia>> ListaPendencias;

// Código objeto final que será escrito nos arquivos
vector<int> CodigoObjeto;

// --- FUNÇÕES AUXILIARES ---

// Converte uma string para maiúsculas
string paraMaiusculo(string s) {
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// Preenche a Tabela de Opcodes com base na imagem
void setupTabelaOpcodes() {
    TabelaOpcodes["ADD"]    = {1, 2};
    TabelaOpcodes["SUB"]    = {2, 2};
    TabelaOpcodes["MUL"]    = {3, 2};
    TabelaOpcodes["DIV"]    = {4, 2};
    TabelaOpcodes["JMP"]    = {5, 2};
    TabelaOpcodes["JMPN"]   = {6, 2};
    TabelaOpcodes["JMPP"]   = {7, 2};
    TabelaOpcodes["JMPZ"]   = {8, 2};
    TabelaOpcodes["COPY"]   = {9, 3};
    TabelaOpcodes["LOAD"]   = {10, 2};
    TabelaOpcodes["STORE"]  = {11, 2};
    TabelaOpcodes["INPUT"]  = {12, 2};
    TabelaOpcodes["OUTPUT"] = {13, 2};
    TabelaOpcodes["STOP"]   = {14, 1};
}

// Verifica se um rótulo é válido
bool rotuloValido(const string& label) {
    if (isdigit(label[0])) {
        return false; // Não pode começar com número
    }
    for (char c : label) {
        if (!isalnum(c) && c != '_') {
            return false; // Só pode ter alfanuméricos e _
        }
    }
    return true;
}

/**
 * Processa um token de operando (ex: "VAR" ou "VAR+1"),
 * resolve o endereço se possível, ou adiciona na ListaPendencias.
 */
void processarOperando(const string& tokenOperando, int& LC) {
    string rotuloBase = paraMaiusculo(tokenOperando);
    int deslocamento = 0;

    // Tenta encontrar o '+' para o deslocamento
    size_t posSoma = tokenOperando.find('+');
    
    if (posSoma != string::npos) {
        // Encontrou um '+', divide o token
        rotuloBase = paraMaiusculo(tokenOperando.substr(0, posSoma));
        
        try {
            deslocamento = stoi(tokenOperando.substr(posSoma + 1));
        } catch (...) {
            // Ex: "NUM+ABC" -> Erro
            cerr << "Erro Sintático: Deslocamento inválido em '" << tokenOperando << "'" << endl;
        }
    }

    if (!rotuloValido(rotuloBase)) {
         cerr << "Erro Léxico: Rótulo '" << rotuloBase << "' inválido no operando." << endl;
    }

    // Agora, usar 'rotuloBase' e 'deslocamento'
    if (TabelaSimbolos.count(rotuloBase)) {
        // Símbolo JÁ conhecido (passado)
        // Resolve o endereço imediatamente
        CodigoObjeto.push_back(TabelaSimbolos[rotuloBase] + deslocamento);
    } else {
        // Símbolo AINDA NÃO conhecido (futuro)
        // Adiciona placeholder 0 para o .o1
        CodigoObjeto.push_back(0); 
        // Adiciona na lista de pendências (com o deslocamento) para o .o2
        ListaPendencias[rotuloBase].push_back({(int)CodigoObjeto.size() - 1, deslocamento}); 
    }
    
    // O operando ocupa 1 palavra no código objeto
    LC++;
}

// --- FUNÇÃO PRINCIPAL DE PROCESSAMENTO ---

int main(int argc, char *argv[]) {
    // 1. Verificação de Argumentos e Arquivos
    if (argc < 2) {
        cerr << "Erro: é preciso um arquivo de entrada." << endl;
        cerr << "Uso: ./compilador arquivo.asm" << endl;
        return 1;
    }

    string nomeArquivoAsm = argv[1];
    // Pega o nome base do arquivo (ex: "arquivo")
    string nomeBase = nomeArquivoAsm.substr(0, nomeArquivoAsm.find_last_of('.'));
    
    ifstream arquivo(nomeArquivoAsm);
    if (!arquivo.is_open()) {
        cerr << "Erro: Não foi possível abrir o arquivo de entrada: " << nomeArquivoAsm << endl;
        return 1;
    }

    // Abre o arquivo de saída .o1
    ofstream arquivo_o1(nomeBase + ".o1");
    if (!arquivo_o1.is_open()) {
        cerr << "Erro: Não foi possível criar o arquivo de saída .o1" << endl;
        return 1;
    }

    // --- NOVO ---
    // Abre o arquivo de saída .o2
    ofstream arquivo_o2(nomeBase + ".o2");
    if (!arquivo_o2.is_open()) {
        cerr << "Erro: Não foi possível criar o arquivo de saída .o2" << endl;
        return 1;
    }
    // --- NOVO ---

    // 2. Inicialização
    setupTabelaOpcodes();
    int LC = 0; // Location Counter (Contador de Posição)
    int contadorLinha = 0;
    string ultimoLabel = ""; // Para tratar "ROT:\n ADD N1" 

    string linha;

    // 3. ALGORITMO DE PASSAGEM ÚNICA
    while (getline(arquivo, linha)) {
        contadorLinha++;
        stringstream ss(linha); // Para "quebrar" a linha em palavras
        string token;
        vector<string> tokens;

        // Transforma a linha em um vetor de tokens (palavras)
        // Isso ignora automaticamente espaços e tabs desnecessários 
        while (ss >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            continue; // Ignora linha em branco
        }

        // --- INÍCIO DA LÓGICA DE PARSING ---

        string label = "";
        int tokenOffset = 0; // Quantos tokens "pular" (ex: o rótulo)

        // 3.1. Verifica se o primeiro token é um Rótulo
        if (tokens[0].back() == ':') {
            label = paraMaiusculo(tokens[0].substr(0, tokens[0].length() - 1));
            tokenOffset = 1;

            if (!rotuloValido(label)) {
                // Erro Léxico
                cerr << "Erro Léxico (linha " << contadorLinha << "): Rótulo '" << label << "' inválido." << endl;
            }
            if (tokens.size() > 1 && tokens[1].back() == ':') {
                // Erro Sintático
                cerr << "Erro Sintático (linha " << contadorLinha << "): Dois rótulos na mesma linha." << endl;
            }
        }

        // 3.2. Trata Rótulos
        // Havia um rótulo na linha anterior? 
        if (!ultimoLabel.empty()) {
            if (!label.empty()) {
                // Ex: ROT1: \n ROT2: ... -> Erro, dois rótulos para a mesma instrução?
                cerr << "Erro Semântico (linha " << contadorLinha << "): Rótulo '" << label << "' segue rótulo da linha anterior '" << ultimoLabel << "'." << endl;
            }
            
            if (TabelaSimbolos.count(ultimoLabel)) {
                // Erro Semântico
                cerr << "Erro Semântico (linha " << contadorLinha-1 << "): Rótulo '" << ultimoLabel << "' declarado duas vezes." << endl;
            } else {
                TabelaSimbolos[ultimoLabel] = LC;
                // NOTA: Aqui ocorreria a resolução de pendências para o .o2
                // ResolverPendencias(ultimoLabel, LC); 
            }
            ultimoLabel = ""; // Consome o rótulo
        }

        // Esta linha continha um rótulo?
        if (!label.empty()) {
            if (tokens.size() == 1) { // Linha SÓ tinha o rótulo
                ultimoLabel = label;
                continue; // Vai para a próxima linha
            } else { // Rótulo E instrução na mesma linha
                if (TabelaSimbolos.count(label)) {
                    // Erro Semântico
                    cerr << "Erro Semântico (linha " << contadorLinha << "): Rótulo '" << label << "' declarado duas vezes." << endl;
                } else {
                    TabelaSimbolos[label] = LC;
                    // NOTA: Aqui ocorreria a resolução de pendências para o .o2
                    // ResolverPendencias(label, LC);
                }
            }
        }

        // Se não há mais tokens, a linha era só um label (ou vazia)
        if (tokenOffset >= tokens.size()) {
            continue;
        }

        // 3.3. Trata Instruções e Diretivas
        string instrucao = paraMaiusculo(tokens[tokenOffset]);

        if (TabelaOpcodes.count(instrucao)) {
            // É uma INSTRUÇÃO
            int opcode = TabelaOpcodes[instrucao].first;
            int tamanho = TabelaOpcodes[instrucao].second;
            int numOperandos = tamanho - 1;

            if (tokens.size() - tokenOffset - 1 != numOperandos) {
                // Erro Semântico
                cerr << "Erro Semântico (linha " << contadorLinha << "): Instrução '" << instrucao << "' espera " << numOperandos << " operandos, mas recebeu " << (tokens.size() - tokenOffset - 1) << "." << endl;
                continue;
            }

            CodigoObjeto.push_back(opcode);
            LC++;

            // Processa os operandos
            for (int i = 0; i < numOperandos; i++) {
                string operando = tokens[tokenOffset + 1 + i];
                // Chama a nova função de processamento
                processarOperando(operando, LC); 
            }

        } else if (instrucao == "SPACE") {
            // É uma DIRETIVA
            int numPalavras = 1; // Padrão
            if (tokens.size() - tokenOffset - 1 > 0) {
                // `SPACE` pode ter argumento
                try {
                    numPalavras = stoi(tokens[tokenOffset + 1]);
                } catch (...) {
                    cerr << "Erro Sintático (linha " << contadorLinha << "): Operando de SPACE deve ser um número." << endl;
                }
            }
            
            for(int i = 0; i < numPalavras; i++) {
                CodigoObjeto.push_back(0);
                LC++;
            }

        } else if (instrucao == "CONST") {
            // É uma DIRETIVA
            if (tokens.size() - tokenOffset - 1 != 1) {
                cerr << "Erro Semântico (linha " << contadorLinha << "): CONST espera 1 operando." << endl;
                continue;
            }
            
            try {
                int valor = stoi(tokens[tokenOffset + 1]);
                CodigoObjeto.push_back(valor);
                LC++;
            } catch (...) {
                cerr << "Erro Sintático (linha " << contadorLinha << "): Operando de CONST deve ser um número." << endl;
            }
            
        } else {
            // Erro Sintático
            cerr << "Erro Sintático (linha " << contadorLinha << "): Instrução '" << instrucao << "' inexistente." << endl;
        }
    }
// 4. VERIFICAÇÃO FINAL (Pós-Passagem)
    bool encontrouErroPendente = false;
    for (auto const& [rotulo, listaDePendencias] : ListaPendencias) {
        if (!TabelaSimbolos.count(rotulo)) {
            // Erro Semântico: Rotulo não declarado
            encontrouErroPendente = true;
            for(const Pendencia& p : listaDePendencias) {
                cerr << "Erro Semântico: Rótulo '" << rotulo << "' não foi declarado (usado no endereço " << p.enderecoNoCodigo << ")." << endl;
            }
        }
    }

    // 5. GERAÇÃO DO ARQUIVO .o1
    // A saída .o1 é o código objeto ANTES de corrigir as pendências
    // em uma única linha, com espaços
    for (size_t i = 0; i < CodigoObjeto.size(); ++i) {
        arquivo_o1 << CodigoObjeto[i] << (i == CodigoObjeto.size() - 1 ? "" : " ");
    }
    arquivo_o1 << endl; 

    cout << "Arquivo .o1 gerado com sucesso!" << endl;

    // --- NOVO ---
    // 6. GERAÇÃO DO ARQUIVO .o2
    // A saída .o2 é o código objeto APÓS corrigir as pendências
    if (encontrouErroPendente) {
        cout << "Erros semânticos (Rótulo não declarado) encontrados. O arquivo .o2 não será gerado corretamente." << endl;
    }

    // Cria uma cópia do código para corrigir
    vector<int> CodigoObjetoCorrigido = CodigoObjeto;

    // Itera sobre a Lista de Pendências para corrigir o código
    for (auto const& [rotulo, listaDePendencias] : ListaPendencias) {
        if (TabelaSimbolos.count(rotulo)) {
            // Se o rótulo foi encontrado na Tabela de Símbolos
            int enderecoCorreto = TabelaSimbolos[rotulo];
            
            for (const Pendencia& p : listaDePendencias) {
                // Aplica a correção: Endereço do Rótulo + Deslocamento
                CodigoObjetoCorrigido[p.enderecoNoCodigo] = enderecoCorreto + p.deslocamento;
            }
        }
        // Se o rótulo não foi encontrado, o '0' (ou o que quer que estivesse lá)
        // permanece, pois o erro já foi reportado.
    }

    // Escreve o vetor CORRIGIDO no arquivo .o2
    for (size_t i = 0; i < CodigoObjetoCorrigido.size(); ++i) {
        arquivo_o2 << CodigoObjetoCorrigido[i] << (i == CodigoObjetoCorrigido.size() - 1 ? "" : " ");
    }
    arquivo_o2 << endl;
    
    cout << "Arquivo .o2 gerado com sucesso!" << endl;
    // --- FIM NOVO ---

    // 7. Limpeza
    arquivo.close();
    arquivo_o1.close();
    arquivo_o2.close(); // --- NOVO ---

    return 0;
}