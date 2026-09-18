# Diagramas de Atividades — Decifra.IA

Diagramas em [Mermaid](https://mermaid.js.org/), renderizados automaticamente pelo GitHub — sem necessidade de imagens exportadas.

---

## US01 — Introdução narrativa
```mermaid
flowchart TD
    A([Início]) --> B{Primeira vez jogando?}
    B -- Sim --> C[Exibir texto de lore]
    B -- Não --> D{Jogador deseja pular?}
    D -- Sim --> E[Pular introdução]
    D -- Não --> C
    C --> F[Liberar Ato 1]
    E --> F
    F --> G([Fim])
```

---

## US02 — Quiz conceitual (Manual de Treinamento)
```mermaid
flowchart TD
    A([Início]) --> B[Exibir pergunta do quiz]
    B --> C[Jogador responde pergunta]
    C --> D{Resposta correta?}
    D -- Sim --> E[Exibir feedback positivo]
    D -- Não --> F[Exibir feedback com explicação]
    E --> G{Existem mais perguntas?}
    F --> G
    G -- Sim --> B
    G -- Não --> H[Exibir resultado final do quiz]
    H --> I{Todos os quizzes do Ato 1 concluídos?}
    I -- Sim --> J[Desbloquear Ato 2]
    I -- Não --> K([Fim])
    J --> K
```

---

## US03 — Desbloqueio de níveis por progresso
```mermaid
flowchart TD
    A([Início]) --> B{Quiz do nível atual concluído?}
    B -- Sim --> C[Liberar acesso ao próximo nível]
    B -- Não --> D[Bloquear acesso]
    D --> E[Exibir mensagem explicativa]
    C --> F([Fim])
    E --> F
```

---

## US04 — Geração de chamados por turno
```mermaid
flowchart TD
    A([Início do turno]) --> B[Sortear quantidade de chamados 1-3]
    B --> C[Gerar chamados novos]
    C --> D[Adicionar à fila mantendo pendentes]
    D --> E([Fim])
```

---

## US05 — Fila de priorização de chamados
```mermaid
flowchart TD
    A([Início]) --> B[Exibir fila de chamados pendentes]
    B --> C[Jogador seleciona um chamado]
    C --> D[Abrir ticket completo]
    D --> E([Fim])
```

---

## US06 — Exibição do ticket de chamado
```mermaid
flowchart TD
    A([Início]) --> B[Carregar dados do chamado]
    B --> C[Exibir nome do cidadão]
    C --> D[Exibir resumo do problema]
    D --> E[Exibir opções de resposta 2-4]
    E --> F([Fim])
```

---

## US07 — Escolha de resposta com custo/benefício
```mermaid
flowchart TD
    A([Início]) --> B[Jogador seleciona opção de resposta]
    B --> C[Consultar regra de efeito da opção]
    C --> D[Aplicar efeito no medidor]
    D --> E{Opção possui efeito colateral?}
    E -- Sim --> F[Registrar efeito colateral]
    E -- Não --> G([Fim])
    F --> G
```

---

## US08 — Feedback imediato após decisão
```mermaid
flowchart TD
    A([Início]) --> B[Processar escolha do jogador]
    B --> C[Montar mensagem de feedback]
    C --> D[Exibir feedback textual]
    D --> E[Retornar à fila de chamados]
    E --> F([Fim])
```

---

## US09 — Medidor de Confiança Pública
```mermaid
flowchart TD
    A([Início]) --> B[Decisão altera o medidor]
    B --> C[Recalcular valor do medidor]
    C --> D{Valor > 100?}
    D -- Sim --> E[Limitar a 100]
    D -- Não --> F{Valor < 0?}
    F -- Sim --> G[Limitar a 0]
    F -- Não --> H[Manter valor]
    E --> I[Atualizar exibição do medidor]
    G --> I
    H --> I
    I --> J([Fim])
```

---

## US10 — Resumo do dia (dashboard ASCII)
```mermaid
flowchart TD
    A([Início]) --> B[Contar chamados atendidos no período]
    B --> C{Atingiu N chamados?}
    C -- Sim --> D[Calcular variação do medidor]
    D --> E[Montar dashboard ASCII]
    E --> F[Exibir resumo do dia]
    C -- Não --> G[Continuar sem exibir]
    F --> H([Fim])
    G --> H
```

---

## US11 — Persistência de progresso em arquivo binário
```mermaid
flowchart TD
    A([Ação relevante concluída]) --> B{Ação foi correta/boa?}
    B -- Sim --> C[Pontuação +1]
    B -- Não --> D[Pontuação -1]
    C --> E[Montar struct de estado]
    D --> E
    E --> F[Gravar struct no arquivo binário]
    F --> G([Fim])

    H([Reabrir o jogo]) --> I[Ler arquivo binário]
    I --> J{Número mágico/checksum válido?}
    J -- Sim --> K[Carregar nível, medidor, pontuação, histórico]
    J -- Não --> L[Tratar erro com segurança]
    L --> M[Iniciar novo progresso zerado]
    K --> N([Fim])
    M --> N
```

---

## US12 — Gatilho narrativo do escândalo (Ato 3)
```mermaid
flowchart TD
    A([Início]) --> B{Marco de progresso do Ato 2 atingido?}
    B -- Sim --> C[Apresentar chamado do escândalo]
    C --> D[Bloquear fila normal]
    D --> E[Direcionar ao mini-jogo de viés]
    B -- Não --> F[Continuar rotina normal]
    E --> G([Fim])
    F --> G
```

---

## US13 — Mini-jogo de viés: montagem do dataset
```mermaid
flowchart TD
    A([Início]) --> B[Exibir lista de exemplos disponíveis]
    B --> C[Jogador inclui/exclui exemplos]
    C --> D[Registrar composição do dataset]
    D --> E{Deseja continuar editando?}
    E -- Sim --> B
    E -- Não --> F[Avançar para etapa de resultado]
    F --> G([Fim])
```

---

## US14 — Mini-jogo de viés: visualização do resultado
```mermaid
flowchart TD
    A([Início]) --> B[Receber composição do dataset]
    B --> C[Rodar simulação determinística]
    C --> D[Calcular taxa de erro/viés por grupo]
    D --> E[Exibir resultado da simulação]
    E --> F([Fim])
```

---

## US15 — Conclusão do Ato 3 e efeito no medidor
```mermaid
flowchart TD
    A([Início]) --> B[Avaliar qualidade da investigação]
    B --> C[Calcular ajuste no medidor]
    C --> D[Aplicar ajuste ao medidor]
    D --> E[Exibir feedback explicando o motivo]
    E --> F([Fim])
```

---

## US16 — Cálculo do final (Ato 4)
```mermaid
flowchart TD
    A([Ato 3 concluído]) --> B[Consultar valor final do medidor]
    B --> C{Faixa do medidor?}
    C -- 0-25 --> D[Selecionar Final 1]
    C -- 26-50 --> E[Selecionar Final 2]
    C -- 51-75 --> F[Selecionar Final 3]
    C -- 76-100 --> G[Selecionar Final 4]
    D --> H([Fim])
    E --> H
    F --> H
    G --> H
```

---

## US17 — Exibição do final da história
```mermaid
flowchart TD
    A([Início]) --> B[Carregar texto do final determinado]
    B --> C[Exibir texto do final]
    C --> D[Exibir resumo estatístico das decisões]
    D --> E[Oferecer reiniciar ou encerrar]
    E --> F([Fim])
```

---

## US18 — Reinício de partida com progresso zerado
```mermaid
flowchart TD
    A([Jogador escolhe nova partida]) --> B{Existe save anterior?}
    B -- Sim --> C[Avisar sobre sobrescrita]
    C --> D{Jogador confirma?}
    D -- Sim --> E[Zerar medidor, pontuação e progresso]
    D -- Não --> F([Fim - cancelado])
    B -- Não --> E
    E --> G[Salvar novo estado inicial]
    G --> H([Fim])
```
