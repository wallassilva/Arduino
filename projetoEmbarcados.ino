//Projeto IRRIGADOR INTELIGENTE

#include <LiquidCrystal_I2C.h>  // Biblioteca utilizada para fazer a comunicação com o display
#include <EEPROM.h>            // Biblioteca para o uso da memória EEPROM

//Definição do display
#define col 16                          // Serve para definir o numero de colunas do display utilizado
#define lin 2                           // Serve para definir o numero de linhas do display utilizado
#define ende 0x27                       // Serve para definir o endereço do display.

// Define os pinos e constantes
#define POTENCIA A0               // Pino conectado ao potenciômetro
#define SENSOR A1                 // Pino conectado ao sensor de umidade do solo
#define LED 8                     // Pino conectado ao LED indicador de necessidade de irrigação
#define POTENCIOMETRO A2          // Pino conectado ao potenciômetro
#define BOTAO_MENU 2              // Pino conectado ao botão de menu
#define BOTAO_ANTERIOR 3          // Pino conectado ao botão anterior
#define BOTAO_PROXIMO 4           // Pino conectado ao botão próximo
#define BOTAO_SALVAR 7            // Pino conectado ao botão de salvar
#define EEPROM_ADDRESS_UMIDADE 0  // Endereço inicial na EEPROM para armazenar o histórico de umidade
#define NUMERO_DIAS 30            // Número de dias para armazenar o histórico de umidade
#define VALOR_MAX_SENSOR 1023     // Valor máximo do sensor ajustado para 100

LiquidCrystal_I2C lcd(ende, col, lin);  // Chamada da função LiquidCrystal para ser usada com o I2C

int historicoUmidade[NUMERO_DIAS];   // Array para armazenar o histórico de umidade
bool modoHistorico = false;          // Variável booleana para indicar o modo de exibição (histórico ou exibição atual)
int diaSelecionado = 0;              // Índice do dia selecionado no histórico

int valorMIN = 0;                    // Valor mínimo do potenciômetro que define o limite de umidade
int valorMAX = 600;                  // Valor máximo do potenciômetro que define o limite de umidade
int limite_umidade = 0;              // Limite de umidade configurado
int ultimoValorPotenciometro = 0;    // Último valor lido do limite de umidade configurado

int umidadeAnterior = 0;             // Último valor de umidade lido por o sensor de umidade
int valorUmidade = 0;                //valor de umidade
int ultimoUmidadePorcentagemExibido = 0; ////valor de umidade em porcentagem

unsigned long lastDebounceTime = 0;  // Último tempo de debounce
const long debounceDelay = 200;      // Tempo de debounce ou seja tempo necessário para validação de um estadao do botão

int ultimoLimiteUmidadeExibido = -1;  // Último limite de umidade exibido
int ultimaUmidadeExibida = -1;        // Última umidade exibida



void setup() {
  Serial.begin(9600);  // Inicializa a comunicação serial

  lcd.init();
  lcd.backlight(); // Serve para ligar a luz do display
  lcd.clear();
  
  // Configuração dos pinos
  pinMode(POTENCIA, OUTPUT);              // Define o pino do potenciômetro como saída
  pinMode(SENSOR, INPUT);                 // Define o pino do sensor de umidade como entrada
  pinMode(LED, OUTPUT);                   // Define o pino do LED como saída
  pinMode(BOTAO_MENU, INPUT_PULLUP);      // Define o pino do botão de menu como entrada com pull-up interno habilitado
  pinMode(BOTAO_PROXIMO, INPUT_PULLUP);   // Define o pino do botão próximo como entrada com pull-up interno habilitado
  pinMode(BOTAO_ANTERIOR, INPUT_PULLUP);  // Define o pino do botão anterior como entrada com pull-up interno habilitado
  pinMode(BOTAO_SALVAR, INPUT_PULLUP);    // Define o pino do botão de salvar como entrada com pull-up interno habilitado

  // Inicializa o display LCD
  inicializarDisplay();

  // Inicializa os valores do histórico a partir da EEPROM
  inicializarHistorico();

  // Lê o valor do potenciômetro e mapeia para a faixa de 0 a 100%
  limite_umidade = map(analogRead(POTENCIOMETRO), 0, VALOR_MAX_SENSOR, valorMIN, valorMAX);
  ultimoValorPotenciometro = limite_umidade;

  // Salva o histórico de umidade e imprime na serial
  imprimirHistoricoUmidade();
}

void loop() {
  // Verifica se o botão de menu foi pressionado e alterna entre os modos de exibição
  if (debounce(BOTAO_MENU)) {
    modoHistorico = !modoHistorico;
    exibirModo();

    // Se o modo de exibição foi ativado, exibe os valores atuais
    if (!modoHistorico) {
      valorUmidade = medirUmidade();
      int umidadePorcentagem = calcularPorcentagem(valorUmidade, limite_umidade);
      exibirValores(limite_umidade, umidadePorcentagem, valorUmidade); // Passa o valor da umidade atual
    }
  }

  int umidadeAtual = medirUmidade();
  int valorPotenciometro = analogRead(POTENCIOMETRO);

  // Atualiza a exibição com base nos valores lidos
  if (!modoHistorico) {
    atualizarExibicao(valorPotenciometro, umidadeAtual);
  } else {
    gerenciarHistorico();
  }

  delay(100);
}

// Função para inicializar o display LCD
void inicializarDisplay() {
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IRRIGADOR");
  lcd.setCursor(0, 1);
  lcd.print("INTELIGENTE");
  delay(1500);
}

// Função para inicializar o histórico a partir da EEPROM
void inicializarHistorico() {
  const int FLAG_ADDRESS = EEPROM_ADDRESS_UMIDADE - 2;
  const byte INITIALIZED_FLAG = 0xFF;

  if (EEPROM.read(FLAG_ADDRESS) != INITIALIZED_FLAG) {
    for (int i = 0; i < NUMERO_DIAS; i++) {
      historicoUmidade[i] = 0;
      EEPROM.put(EEPROM_ADDRESS_UMIDADE + i * sizeof(int), historicoUmidade[i]);
    }

    EEPROM.write(FLAG_ADDRESS, INITIALIZED_FLAG);
  } else {
    for (int i = 0; i < NUMERO_DIAS; i++) {
      historicoUmidade[i] = EEPROM.get(EEPROM_ADDRESS_UMIDADE + i * sizeof(int), historicoUmidade[i]);
    }
  }
}

// Função para imprimir o histórico de umidade na porta serial
void imprimirHistoricoUmidade() {
  Serial.println("Historico de Umidade:");
  for (int i = 0; i < NUMERO_DIAS; i++) {
    Serial.print("Umidade do dia ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(historicoUmidade[i]);
  }
}

// Função para realizar debounce em um pino de entrada
bool debounce(int pin) {
  int leitura = digitalRead(pin);
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (leitura == LOW) {
      lastDebounceTime = millis();
      return true;
    }
  }
  return false;
}

// Função para exibir o modo de exibição atual no display LCD
void exibirModo() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MODO");
  lcd.setCursor(0, 1);
  lcd.print(modoHistorico ? "HISTORICO" : "EXIBICAO");
  delay(500);
}

// Função para medir a umidade
int medirUmidade() {
  digitalWrite(POTENCIA, HIGH);
  delay(10);
  int umidade = analogRead(SENSOR);
  digitalWrite(POTENCIA, LOW);
  
  //O valor fornecido no sensor não é o de umidade mas sim o de escassez
  umidade = VALOR_MAX_SENSOR - umidade;

  return umidade;
}

// Função para calcular a porcentagem de umidade
int calcularPorcentagem(int umidade, int limite) {
  return abs((umidade * 100) / limite);
}

// Função para exibir os valores de limite de umidade, porcentagem e umidade real no display
void exibirValores(int limite, int umidadePorcentagem, int umidadeReal) {
  // Verifica se os novos valores diferem dos últimos valores exibidos
  if (abs(limite - ultimoLimiteUmidadeExibido) > 10 || abs(umidadePorcentagem - ultimoUmidadePorcentagemExibido) > 10) {
    // Atualiza a exibição no LCD com os novos valores
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Limite: ");
    lcd.print(limite);

    lcd.setCursor(0, 1);
    lcd.print("Umidade: ");
    lcd.print(abs(umidadePorcentagem));
    lcd.print("%");


    delay(500);


  if (umidadeReal < limite) {
    digitalWrite(LED, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PRECISA IRRIGAR");
  } else {
    digitalWrite(LED, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HIDRATADA ");
  }

    // Atualiza as variáveis de último valor exibido com os novos valores
    ultimoLimiteUmidadeExibido = limite;
    ultimoUmidadePorcentagemExibido = umidadePorcentagem;
    


  }
}

// Função para atualizar a exibição no display com base nos valores lidos
void atualizarExibicao(int valorPotenciometro, int umidadeAtual) {
  if (valorPotenciometro != ultimoValorPotenciometro || umidadeAtual != umidadeAnterior) {
    limite_umidade = map(valorPotenciometro, 0, VALOR_MAX_SENSOR, valorMIN, valorMAX);

    exibirValores(limite_umidade, calcularPorcentagem(umidadeAtual, limite_umidade), umidadeAtual);

    ultimoValorPotenciometro = valorPotenciometro;
    umidadeAnterior = umidadeAtual;
  }
}

// Função para gerenciar o histórico
void gerenciarHistorico() {
  if (debounce(BOTAO_PROXIMO)) {
    diaSelecionado = (diaSelecionado + 1) % NUMERO_DIAS;
    exibirHistorico();
  }

  if (debounce(BOTAO_ANTERIOR)) {
    diaSelecionado = (diaSelecionado - 1 + NUMERO_DIAS) % NUMERO_DIAS;
    exibirHistorico();
  } else {
  if (debounce(BOTAO_SALVAR)) {
    salvarHistorico();
    exibirHistorico();
  }
    }
}

// Função para salvar o histórico de umidade na EEPROM
void salvarHistorico() {
  int umidadeAtual = medirUmidade();
  int umidadePorcentagem = calcularPorcentagem(umidadeAtual, limite_umidade);
  historicoUmidade[diaSelecionado] = umidadePorcentagem; // Salva a umidade medida no histórico

  EEPROM.put(EEPROM_ADDRESS_UMIDADE + diaSelecionado * sizeof(int), historicoUmidade[diaSelecionado]); // Salva o histórico na EEPROM

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Salvando...");
  delay(1000);  // Delay para exibir a mensagem "Salvando..."

  Serial.println("");
  Serial.println("Atualizacao:");
  Serial.print("Umidade do dia ");
  Serial.print(diaSelecionado + 1);
  Serial.print(": ");
  Serial.println(umidadePorcentagem); // Imprime a umidade medida
}

// Função para exibir o histórico de umidade no display
void exibirHistorico() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Historico:");
  lcd.setCursor(0, 1);
  lcd.print("Dia ");
  lcd.print(diaSelecionado + 1);
  lcd.print(": ");
  lcd.print(historicoUmidade[diaSelecionado]);
  lcd.print("%");
}
