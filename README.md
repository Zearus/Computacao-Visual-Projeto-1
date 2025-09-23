# Projeto 1 – Processamento de Imagens (Computação Visual)

## Autores: 
  João Victor Dallapé Madeira RA: 10400725

##  Objetivos do projeto

De acordo com o enunciado, o programa deveria:

1. Carregar imagens nos formatos mais comuns (PNG, JPG, BMP).
2. Converter a imagem para **escala de cinza** usando a fórmula:
   ```
   Y = 0.2125 * R + 0.7154 * G + 0.0721 * B
   ```
3. Abrir duas janelas:
   - **Janela principal:** exibe a imagem processada.
   - **Janela secundária:** exibe o **histograma** e contém um **botão de equalização**.
4. Calcular e exibir informações estatísticas:
   - Intensidade → classificar como **clara**, **média** ou **escura**.
   - Contraste → classificar como **alto**, **médio** ou **baixo**.
5. Implementar a **equalização do histograma**, com botão que alterna entre:
   - Mostrar a imagem original.
   - Mostrar a versão equalizada.
   - O botão muda de cor conforme interação do usuário (neutro, hover, clique).
6. Permitir salvar a imagem atual com a tecla **S** (gerando `output_image.png`).

---

##  Como funciona

Quando o programa é executado, ele:

1. **Carrega a imagem** passada por argumento.
   ```bash
   ./main caminho_da_imagem.png
   ```

2. **Abre duas janelas:**
   - A primeira mostra a imagem (em escala de cinza ou equalizada).
   - A segunda mostra o histograma + botão de equalização + classificação de intensidade/contraste.

3. **Interações possíveis:**
   - **Clique no botão:** alterna entre a imagem original e a equalizada.
   - **Pressione ESC:** fecha o programa.
   - **Pressione S:** salva a imagem atual como `output_image.png`.

---

##  Compilação e execução

Pré-requisitos:
- **gcc** (C99 ou superior)
- **SDL3**, **SDL3_image** e **SDL3_ttf** instalados

Comando de compilação usado no Windows (MinGW):

```bash
gcc main.c -o main -Ic:/libs/SDL3/include -Lc:/libs/SDL3/lib -lSDL3 -lSDL3_image -lSDL3_ttf -lm
```

Execução:

```bash
./main imagem.png
```

---

##  Estrutura do código

- **Estrutura `GrayImage`:** guarda largura, altura e pixels em escala de cinza.
- **Funções principais:**
  - `load_and_convert_to_gray()` → carrega e converte imagem para grayscale.
  - `compute_histogram()` → calcula histograma, média e desvio padrão.
  - `equalize_histogram()` → gera nova imagem equalizada.
  - `save_gray_png()` → salva a imagem atual em PNG.
- **Loop principal:**
  - Renderiza a imagem.
  - Atualiza o histograma.
  - Desenha o botão de equalização.
  - Exibe classificações (intensidade e contraste).

---

##  Observação
  Para execução do código main era necessário os arquivos no diretório do código main: SDL3.dll SDL3_image.dll SDL3_ttf.dll e o arial.ttf para fazer as legendas funcionarem. 

---

## Print da execução

<img width="1404" height="863" alt="image" src="https://github.com/user-attachments/assets/e1adb196-1bb6-4d26-8e17-63887fe7829c" />

