#include <bits/stdc++.h>

using namespace std;

// =========================================================================
// ESTRUTURAS GLOBAIS (Usadas nas duas fases)
// =========================================================================

// Tabela de Opcodes (Instrução -> {Opcode, Tamanho em palavras})
map<string, pair<int, int>> TabelaOpcodes;

// --- ESTRUTURAS DA FASE 1 (PRÉ-PROCESSAMENTO) ---
struct Macro {
    string nome;
    vector<string> args;
    vector<string> corpo; // Vetor de linhas
};
map<string, Macro> TabelaMacros; // Nossa MNT (Tabela de Nomes de Macros)

// --- ESTRUTURAS DA FASE 2 (COMPILAÇÃO) ---
map<string, int> TabelaSimbolos; // Tabela de Símbolos (Rótulo -> Endereço)
struct Pendencia {
    int enderecoNoCodigo; // Onde o '0' está
    int deslocamento;     // O '1' em 'NUM+1'
};
map<string, vector<Pendencia>> ListaPendencias; // Lista de Pendências
vector<int> CodigoObjeto; // Vetor para .o1 e .o2


// =========================================================================
// FUNÇÕES AUXILIARES GERAIS
// =========================================================================

// Converte uma string para maiúsculas
string paraMaiusculo(string s) {
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// Verifica se um rótulo é válido
bool rotuloValido(const string& label) {
    if (label.empty()) return false;
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


// =========================================================================
// FASE 1: LÓGICA DO PRÉ-PROCESSADOR (.ASM -> .PRE)
// =========================================================================

// Esta função expande uma macro e lida com a substituição de argumentos
vector<string> expandirMacro(Macro& macro, vector<string>& argsRecebidos) {
    vector<string> linhasExpandidas;

    for (string linhaCorpo : macro.corpo) {
        // Para cada linha no corpo da macro, substitui os argumentos
        for (size_t i = 0; i < macro.args.size(); ++i) {
            size_t pos = 0;
            // Usa find e replace para substituir &ARG por ARG_RECEBIDO
            while ((pos = linhaCorpo.find(macro.args[i], pos)) != string::npos) {
                linhaCorpo.replace(pos, macro.args[i].length(), argsRecebidos[i]);
                pos += argsRecebidos[i].length();
            }
        }
        linhasExpandidas.push_back(linhaCorpo);
    }
    return linhasExpandidas;
}

// ### FUNÇÃO CORRIGIDA ###
bool executar_preprocessamento(string arqEntrada, string arqSaida) {
    ifstream arquivo(arqEntrada);
    if (!arquivo.is_open()) {
        cerr << "Erro FASE 1: Não foi possível abrir o arquivo .asm: " << arqEntrada << endl;
        return false;
    }
    
    ofstream arquivo_pre(arqSaida);
    if (!arquivo_pre.is_open()) {
        cerr << "Erro FASE 1: Não foi possível criar o arquivo .pre: " << arqSaida << endl;
        return false;
    }

    list<string> linhasParaProcessar;
    string linha;
    
    while (getline(arquivo, linha)) {
        linhasParaProcessar.push_back(linha);
    }
    arquivo.close();

    while (!linhasParaProcessar.empty()) {
        linha = linhasParaProcessar.front();
        linhasParaProcessar.pop_front();

        size_t posComentario = linha.find(';');
        if (posComentario != string::npos) {
            linha = linha.substr(0, posComentario);
        }

        stringstream ss(linha);
        string token;
        vector<string> tokens;
        while (ss >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            arquivo_pre << endl;
            continue; 
        }

        int tokenOffset = 0;
        if (tokens[0].back() == ':') {
            tokenOffset = 1;
        }

        if (tokenOffset >= tokens.size()) {
            arquivo_pre << linha << endl;
            continue;
        }

        string instrucao = paraMaiusculo(tokens[tokenOffset]);

        // --- Lógica de Definição de Macro ---
        if (instrucao == "MACRO") {
            Macro novaMacro;
            string macroName;
            int argStartIndex;

            // ***** CORREÇÃO AQUI *****
            if (tokenOffset == 1) { // Macro tem rótulo (ex: "RES: MACRO ...")
                macroName = paraMaiusculo(tokens[0].substr(0, tokens[0].length() - 1)); // "RES"
                argStartIndex = tokenOffset + 1; // Args começam no token 2 (ex: &N1)
            } else { // Sem rótulo (ex: "MACRO SWAP ...")
                macroName = paraMaiusculo(tokens[1]); // "SWAP"
                argStartIndex = tokenOffset + 2; // Args começam no token 2 (ex: &A)
            }
            // *******************************

            novaMacro.nome = macroName;
            
            for (size_t i = argStartIndex; i < tokens.size(); ++i) {
                novaMacro.args.push_back(tokens[i]);
            }

            string linhaCorpo;
            while (true) {
                if (linhasParaProcessar.empty()) {
                    cerr << "Erro FASE 1: Fim de arquivo inesperado. Macro '" << novaMacro.nome << "' não foi fechada com ENDMACRO." << endl;
                    return false;
                }
                linhaCorpo = linhasParaProcessar.front();
                linhasParaProcessar.pop_front();
                
                stringstream ssCorpo(linhaCorpo);
                string tokenCorpo;
                ssCorpo >> tokenCorpo;
                if (paraMaiusculo(tokenCorpo) == "ENDMACRO") {
                    break;
                }
                novaMacro.corpo.push_back(linhaCorpo);
            }
            TabelaMacros[novaMacro.nome] = novaMacro;
            continue; 
        }

        // --- Lógica de Expansão de Macro ---
        if (TabelaMacros.count(instrucao)) {
            // É uma chamada de macro!
            Macro macro = TabelaMacros[instrucao];
            vector<string> argsRecebidos;
            for (size_t i = tokenOffset + 1; i < tokens.size(); ++i) {
                argsRecebidos.push_back(tokens[i]);
            }

            if (argsRecebidos.size() > 2) { //
                cout << "Aviso FASE 1: Macro '" << instrucao << "' chamada com mais de 2 argumentos." << endl;
            }

            vector<string> linhasExpandidas = expandirMacro(macro, argsRecebidos);

            // Adiciona as linhas expandidas NO COMEÇO da lista (para macros aninhadas)
            linhasParaProcessar.insert(linhasParaProcessar.begin(), linhasExpandidas.begin(), linhasExpandidas.end());

            // Se a linha original tinha um rótulo, preserva ele
            if (tokenOffset == 1) {
                string rotulo = tokens[0];
                if (!linhasParaProcessar.empty()) {
                    linhasParaProcessar.front() = rotulo + " " + linhasParaProcessar.front();
                } else {
                    // Macro vazia, apenas escreve o rótulo
                    arquivo_pre << rotulo << endl;
                }
            }
            continue;
        }

        // Se não for definição nem chamada de macro, escreve a linha no .pre
        arquivo_pre << linha << endl;
    }

    cout << "FASE 1: Pré-processamento concluído -> " << arqSaida << endl;
    arquivo_pre.close();
    return true;
}


// =========================================================================
// FASE 2: LÓGICA DO COMPILADOR (.PRE -> .O1, .O2)
// (Função 'main' anterior, agora chamada pela 'main' real)
// =========================================================================

bool executar_passagem_unica(string arqEntrada, string arqSaidaO1, string arqSaidaO2, int& contadorLinhaPre) {
    
    ifstream arquivo(arqEntrada);
    if (!arquivo.is_open()) {
        cerr << "Erro FASE 2: Não foi possível abrir o arquivo .pre: " << arqEntrada << endl;
        return false;
    }

    ofstream arquivo_o1(arqSaidaO1);
    if (!arquivo_o1.is_open()) {
        cerr << "Erro FASE 2: Não foi possível criar o arquivo de saída .o1" << endl;
        return false;
    }

    ofstream arquivo_o2(arqSaidaO2);
    if (!arquivo_o2.is_open()) {
        cerr << "Erro FASE 2: Não foi possível criar o arquivo de saída .o2" << endl;
        return false;
    }

    int LC = 0; // Location Counter
    string ultimoLabel = "";
    string linha;

    // 3. ALGORITMO DE PASSAGEM ÚNICA
    while (getline(arquivo, linha)) {
        contadorLinhaPre++; // Incrementa o contador de linhas do .pre
        
        size_t posComentario = linha.find(';');
        if (posComentario != string::npos) {
            linha = linha.substr(0, posComentario);
        }

        stringstream ss(linha);
        string token;
        vector<string> tokens;

        while (ss >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            continue;
        }

        // --- LÓGICA DE PARSING (Idêntica ao seu main.cpp) ---
        string label = "";
        int tokenOffset = 0; 

        if (tokens[0].back() == ':') {
            label = paraMaiusculo(tokens[0].substr(0, tokens[0].length() - 1));
            tokenOffset = 1;

            if (!rotuloValido(label)) {
                cerr << "Erro Léxico (linha .pre " << contadorLinhaPre << "): Rótulo '" << label << "' inválido." << endl; //
            }
            if (tokens.size() > 1 && tokens[1].back() == ':') {
                cerr << "Erro Sintático (linha .pre " << contadorLinhaPre << "): Dois rótulos na mesma linha." << endl; //
            }
        }

        if (!ultimoLabel.empty()) {
            if (!label.empty()) {
                cerr << "Erro Semântico (linha .pre " << contadorLinhaPre << "): Rótulo '" << label << "' segue rótulo da linha anterior '" << ultimoLabel << "'." << endl;
            }
            if (TabelaSimbolos.count(ultimoLabel)) {
                cerr << "Erro Semântico (linha .pre " << contadorLinhaPre-1 << "): Rótulo '" << ultimoLabel << "' declarado duas vezes." << endl; //
            } else {
                TabelaSimbolos[ultimoLabel] = LC;
            }
            ultimoLabel = "";
        }

        if (!label.empty()) {
            if (tokens.size() == 1) { 
                ultimoLabel = label;
                continue; 
            } else {
                if (TabelaSimbolos.count(label)) {
                    cerr << "Erro Semântico (linha .pre " << contadorLinhaPre << "): Rótulo '" << label << "' declarado duas vezes." << endl; //
                } else {
                    TabelaSimbolos[label] = LC;
                }
            }
        }

        if (tokenOffset >= tokens.size()) {
            continue;
        }

        string instrucao = paraMaiusculo(tokens[tokenOffset]);

        if (TabelaOpcodes.count(instrucao)) {
            int opcode = TabelaOpcodes[instrucao].first;
            int tamanho = TabelaOpcodes[instrucao].second;
            int numOperandos = tamanho - 1;

            if (tokens.size() - tokenOffset - 1 != numOperandos) {
                cerr << "Erro Semântico (linha .pre " << contadorLinhaPre << "): Instrução '" << instrucao << "' espera " << numOperandos << " operandos, mas recebeu " << (tokens.size() - tokenOffset - 1) << "." << endl; //
                continue;
            }

            CodigoObjeto.push_back(opcode);
            LC++;

            for (int i = 0; i < numOperandos; i++) {
                string operando = tokens[tokenOffset + 1 + i];
                // Processa "LABEL+N"
                string rotuloBase = paraMaiusculo(operando);
                int deslocamento = 0;
                size_t posSoma = operando.find('+');
                
                if (posSoma != string::npos) {
                    rotuloBase = paraMaiusculo(operando.substr(0, posSoma));
                    try {
                        string strDeslocamento = operando.substr(posSoma + 1);
                        if(strDeslocamento.empty()) {
                             cerr << "Erro Sintático (linha .pre " << contadorLinhaPre << "): Deslocamento vazio em '" << operando << "'" << endl;
                        } else {
                            deslocamento = stoi(strDeslocamento);
                        }
                    } catch (...) {
                         cerr << "Erro Sintático (linha .pre " << contadorLinhaPre << "): Deslocamento inválido em '" << operando << "'" << endl;
                    }
                }

                if (!rotuloValido(rotuloBase)) {
                    cerr << "Erro Léxico (linha .pre " << contadorLinhaPre << "): Rótulo '" << rotuloBase << "' inválido no operando." << endl;
                }

                if (TabelaSimbolos.count(rotuloBase)) {
                    CodigoObjeto.push_back(TabelaSimbolos[rotuloBase] + deslocamento);
                } else {
                    CodigoObjeto.push_back(0); // Placeholder para .o1
                    ListaPendencias[rotuloBase].push_back({LC, deslocamento}); 
                }
                LC++;
            }

        } else if (instrucao == "SPACE") {
            int numPalavras = 1; 
            if (tokens.size() - tokenOffset - 1 > 0) {
                try {
                    numPalavras = stoi(tokens[tokenOffset + 1]);
                } catch (...) {
                    cerr << "Erro Sintático (linha .pre " << contadorLinhaPre << "): Operando de SPACE deve ser um número." << endl;
                }
            }
            for(int i = 0; i < numPalavras; i++) {
                CodigoObjeto.push_back(0); //
                LC++;
            }

        } else if (instrucao == "CONST") {
            if (tokens.size() - tokenOffset - 1 != 1) {
                cerr << "Erro Semântico (linha .pre " << contadorLinhaPre << "): CONST espera 1 operando." << endl;
                continue;
            }
            try {
                int valor = stoi(tokens[tokenOffset + 1]);
                CodigoObjeto.push_back(valor);
                LC++;
            } catch (...) {
                cerr << "Erro Sintático (linha .pre " << contadorLinhaPre << "): Operando de CONST deve ser um número." << endl;
            }
            
        } else {
            // Ignora diretivas de pré-processador que possam ter sobrado
            if (instrucao != "MACRO" && instrucao != "ENDMACRO") {
                 cerr << "Erro Sintático (linha .pre " << contadorLinhaPre << "): Instrução '" << instrucao << "' inexistente." << endl; //
            }
        }
    }
    arquivo.close();

    // 4. VERIFICAÇÃO FINAL E GERAÇÃO DE SAÍDA
    bool encontrouErroPendente = false;
    for (auto const& [rotulo, listaDePendencias] : ListaPendencias) {
        if (!TabelaSimbolos.count(rotulo)) {
            encontrouErroPendente = true;
            for(const Pendencia& p : listaDePendencias) {
                cerr << "Erro Semântico: Rótulo '" << rotulo << "' não foi declarado (usado no endereço .o1 " << p.enderecoNoCodigo << ")." << endl; //
            }
        }
    }

    // 5. GERAÇÃO DO ARQUIVO .o1
    for (size_t i = 0; i < CodigoObjeto.size(); ++i) {
        arquivo_o1 << CodigoObjeto[i] << (i == CodigoObjeto.size() - 1 ? "" : " ");
    }
    arquivo_o1.close();
    cout << "FASE 2: Arquivo .o1 gerado com sucesso!" << endl;

    // 6. GERAÇÃO DO ARQUIVO .o2
    if (encontrouErroPendente) {
        cout << "Aviso FASE 2: Erros semânticos encontrados. O arquivo .o2 pode estar incorreto." << endl;
    }

    vector<int> CodigoObjetoCorrigido = CodigoObjeto;
    for (auto const& [rotulo, listaDePendencias] : ListaPendencias) {
        if (TabelaSimbolos.count(rotulo)) {
            int enderecoCorreto = TabelaSimbolos[rotulo];
            for (const Pendencia& p : listaDePendencias) {
                CodigoObjetoCorrigido[p.enderecoNoCodigo] = enderecoCorreto + p.deslocamento;
            }
        }
    }

    for (size_t i = 0; i < CodigoObjetoCorrigido.size(); ++i) {
        arquivo_o2 << CodigoObjetoCorrigido[i] << (i == CodigoObjetoCorrigido.size() - 1 ? "" : " ");
    }
    arquivo_o2.close();
    cout << "FASE 2: Arquivo .o2 gerado com sucesso!" << endl;

    return true;
}


// =========================================================================
// FUNÇÃO MAIN (CONTROLADOR)
// =========================================================================

int main(int argc, char *argv[]) {
    // 1. Setup
    if (argc < 2) {
        cerr << "Uso: ./compilador arquivo.asm" << endl; //
        return 1;
    }
    setupTabelaOpcodes(); // Preenche a tabela de opcodes

    string nomeArquivoAsm = argv[1];
    string nomeBase = nomeArquivoAsm.substr(0, nomeArquivoAsm.find_last_of('.'));
    
    string arqPre = nomeBase + ".pre";
    string arqO1 = nomeBase + ".o1";
    string arqO2 = nomeBase + ".o2";

    // --- EXECUÇÃO EM CADEIA ---

    // 2. Chama a FASE 1 (Pré-processamento)
    bool pre_ok = executar_preprocessamento(nomeArquivoAsm, arqPre);

    // 3. Se a Fase 1 deu certo, chama a FASE 2 (Compilação)
    if (pre_ok) {
        int contadorLinhaPre = 0; // Para reportar erros na linha correta do .pre
        executar_passagem_unica(arqPre, arqO1, arqO2, contadorLinhaPre);
    } else {
        cerr << "Erro: Falha no pré-processamento. Compilação abortada." << endl;
        return 1;
    }

    return 0;
}