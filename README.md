# Rev.IA — Decifra.IA
 
> Um jogo de terminal sobre alfabetização em Inteligência Artificial.
 
Rev.IA (nome do projeto acadêmico) é um jogo casual em modo texto/terminal cujo objetivo é ensinar, de forma prática e narrativa, como funcionam — e onde falham — os sistemas de Inteligência Artificial presentes no dia a dia: viés algorítmico, alucinações, decisões automatizadas e uso seguro de ferramentas de IA generativa.
 
O jogo é desenvolvido como projeto da disciplina **Projetos 2** (2º período de ADS) da **CESAR School**, integrando também os componentes de Lógica, IHC/HCI e FDS/FP2.
 
---

## Descrição
 
Você é recém-contratado(a) do **Departamento de Alfabetização Algorítmica** de uma cidade fictícia onde uma IA generalista chamada **NÓVA** foi implantada silenciosamente em quase todos os serviços públicos — trânsito, crédito, notícias, saúde. Ninguém entende bem como ela decide.
 
Seu cargo, **Decifrador(a)**, existe para auditar as decisões da NÓVA e educar cidadãos que abrem chamados de reclamação sobre ela.

## Equipe
 
Projeto desenvolvido para a disciplina de Projetos 2 (2º período ADS) — CESAR School, integrando os componentes curriculares de Programação, IHC/HCI, Lógica, FDS e FP2.
 
- **Visão e requisitos:** Carlos Daniel, Davi Braz, Davi Soares, João Victor, José Nalbert, Kauã Mateus, Luis Silva
---
 
### Estrutura narrativa em atos
 
| Ato | Nome | Descrição |
|---|---|---|
| 1 | Onboarding | 4-5 missões de quiz conceitual (o que é IA, dados, treino) — tutorial disfarçado de capacitação obrigatória |
| 2 | Rotina de chamados | O grosso do jogo: turnos diários com 1-3 chamados novos, misturando quiz e cenário de decisão, com fila de prioridade |
| 3 | O escândalo | Um chamado revela viés discriminatório em um subsistema da NÓVA, disparando o mini-jogo de investigação forense |
| 4 | Desfecho | O histórico de decisões (medidor de Confiança Pública) define um dos 3-4 finais possíveis |
 
### Mecânicas principais
 
- **Manual de Treinamento** — quizzes conceituais que desbloqueiam níveis
- **Chamados (tickets)** — cenários de decisão com 2-4 opções de resposta, cada uma com custo/benefício não óbvio
- **Medidor de Confiança Pública (0-100)** — reflete o impacto acumulado das decisões do jogador
- **Mini-jogo de viés** — o jogador monta um dataset de treino e observa o viés se manifestar nos resultados simulados
- **Resumo do dia** — dashboard ASCII exibido a cada N chamados, reforçando a sensação de progresso salvo
---
 
## Personas
 
O design do jogo é guiado por dois perfis-alvo principais:
 
**Camila Souza Albuquerque, 34** — empreendedora, dona de uma pequena loja online. Tem relação intimidada com IA ("acha que é coisa de programador"). Quer entender como o viés algorítmico afeta o alcance dos seus anúncios e como usar IA para gerar imagens com segurança.
 
**Roberto de Lima Barreto, 21** — estudante universitário e estagiário. Relação imediatista com IA: usa para resumir textos e confia cegamente nas respostas, o que já lhe custou notas baixas. Precisa aprender a identificar alucinações da IA e a escrever prompts mais precisos.
 
---
 
## Requisitos
 
### Requisitos Funcionais (RF)
- Narrativa com começo, meio e fim que gere engajamento
- Interface simples e acessível, voltada também a públicos não-nativos digitais
- Tutorial, créditos, configurações e dados salvos
- Sessões de história entre as fases; gameplay primário baseado em desafios conceituais e contextuais
- 3 setores narrativos e 4 sessões de perguntas (10 questões cada, dificuldade crescente)
- Todas as 10 perguntas de uma fase devem estar corretas para avançar; em caso de erro, a fase reinicia com variações mantendo coerência narrativa
### Requisitos Não Funcionais (RNF)
- Desenvolvido em **C** (núcleo) e **Haskell** (módulos de regras), visando estabilidade e ausência de conflitos técnicos
- Salvamento de progresso e feedback para todas as respostas do usuário, corretas ou não
- Compatível com Windows, macOS e Linux (suporte a Android/iOS é opcional)
- Pontuação final de proficiência baseada em tempo de conclusão e quantidade de erros, gerando um arquivo de certificação de conclusão
---
 
## Stack técnica
 
- **C** — núcleo do jogo (loop principal, terminal, persistência em arquivo texto)
- **Haskell** — módulos de regras (lógica de decisão, cálculo de viés, mapeamento de finais)
- Sem banco de dados: persistência via arquivo texto
- Sem engine gráfica nem multiplayer/rede — experiência 100% baseada em terminal
---

## Metodologia
 
O desenvolvimento segue histórias de usuário no formato **3C (Card, Conversation, Confirmation)**, validadas pelo critério **INVEST**, organizadas em um board Scrum (Jira) com sprints alinhadas aos 4 atos do jogo:
 
- Sprint 1 — Ato 1 / Onboarding
- Sprint 2 — Ato 2 / Rotina de Chamados
- Sprint 3 — Ato 3 / Escândalo
- Sprint 4 — Ato 4 / Desfecho
---
