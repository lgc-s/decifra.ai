# Relatório de Teste de Usabilidade

## 1. Identificação do Projeto

* **Nome do Projeto:** Rev.ia

* **Tecnologia / Linguagem:** Desenvolvido em Linguagem C e Raylib

* **Temática:** Alfabetização em Inteligência Artificial (IA)

* **Versão Testada:** Protótipo v3.0

* **Data dos Testes:** 22/11/2026

## 2. Objetivo dos Testes

Avaliar a experiência do usuário, a clareza das instruções na tela, a responsividade dos controles e a compreensão das mecânicas principais do jogo (movimentação, coleta, *dash*, atendimento de chamados e resolução de desafios sobre Inteligência Artificial) por usuários com diferentes níveis de familiaridade com jogos digitais.

## 3. Metodologia Utilizada

Os testes foram realizados de forma presencial e individual, com duração média de \[X\] minutos por sessão.

* **Abordagem:** Teste moderado com observação direta e técnica *Think Aloud* (pensar em voz alta), incentivando os participantes a verbalizarem suas dúvidas e impressões durante a partida.

* **Roteiro:** Cada participante recebeu uma breve contextualização sobre o tema (alfabetização em IA) e foi convidado a executar 7 tarefas essenciais sem intervenção inicial do moderador, permitindo identificar gargalos naturais de usabilidade.

## 4. Perfil dos Três Participantes

| Participante | Perfil | Experiência com jogos | 
| ----- | ----- | ----- | 
| **Danillo** | Estudante universitário | Intermediária | 
| **Beatriz** | Estudante de outro curso | Básica | 
| **Vinicius** | Estudante universitário | Avançada | 

## 5. Resultados das Tarefas

| Tarefa | Danillo | Beatriz | Vinicius | 
| ----- | ----- | ----- | ----- | 
| **Iniciar partida** | Conseguiu | Conseguiu | Conseguiu | 
| **Selecionar personagem** | Fácil | Teve dúvida | Fácil | 
| **Movimentar personagem** | Fácil | Dificuldade inicial | Fácil | 
| **Coletar itens** | Conseguiu | Teve dificuldade | Conseguiu | 
| **Utilizar dash** | Conseguiu | Teve dúvida | Conseguiu | 
| **Atender chamados** | Fácil | Teve dificuldade | Fácil | 
| **Responder desafios** | Conseguiu | Conseguiu | Conseguiu | 

## 6. Problemas Identificados

Com base na observação dos testes (especialmente na experiência de usuários com perfil básico, como a participante Beatriz), foram levantados os seguintes pontos de melhoria:

1. **Seleção de Personagem:** Falta de feedback visual ou textual claro indicando qual tecla confirma a escolha do personagem.

2. **Movimentação e Uso do Dash:** Ausência de um tutorial rápido ou legenda fixa na tela informando as teclas de movimentação (ex: WASD/Setas) e a tecla específica para acionar o *dash*.

3. **Coleta de Itens:** A área de colisão (*hitbox*) ou a indicação de proximidade para coletar os itens de IA estava pouco evidente, gerando dificuldade inicial.

4. **Atendimento de Chamados:** Falta de destaque visual (alerta ou ícone) indicando onde e como interagir para atender aos chamados no cenário.

## 7. Correções Realizadas

Para solucionar os problemas apontados e tornar o jogo mais acessível para iniciantes, foram implementadas as seguintes melhorias no código em C:

* **Melhoria na Seleção:** Inclusão de instruções explícitas na tela de seleção de personagem (ex: *"Use as setas para escolher e pressione ENTER para confirmar"*).

* **Ajuste de Colisão e Feedback:** Ampliação da área de detecção para coleta de itens e adição de mensagens/efeitos visuais de confirmação ao coletar um item ou abrir um chamado.

* **Sinalização de Chamados:** Destaque visual mais claro nos pontos de chamado para guiar o jogador intuitivamente.

## 8. Conclusão

Os testes de usabilidade demonstraram que o núcleo do jogo em C sobre alfabetização em Inteligência Artificial é funcional, envolvente e cumpre seu papel educativo, visto que todos os participantes conseguiram iniciar a partida e responder aos desafios de IA com sucesso.

A inclusão de uma participante com experiência básica em jogos foi fundamental para evidenciar a necessidade de instruções mais claras na interface. Após a aplicação das correções de feedback visual e mapeamento de teclas na tela, o jogo tornou-se mais intuitivo e acessível para diferentes perfis de estudantes.