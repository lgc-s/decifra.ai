#include "narrative/dialogue.h"

#include <stddef.h>

static const Dialogue DIALOGUES[DIALOGUE_COUNT] = {
    /* ---- Ato 1: Onboarding ---- */
    /* 0 */ { 3, {
        { "Supervisão", "Bem-vindo ao Departamento de Alfabetização Algorítmica. Aqui a gente traduz a NÓVA para as pessoas." },
        { "Supervisão", "Você é o nosso novo Decifrador: investiga decisões automatizadas, acha falhas e explica o que descobriu." },
        { "Supervisão", "Aperte E perto de terminais e pessoas para interagir. Explore o departamento antes de sair." } } },
    /* 1 */ { 2, {
        { "Terminal", "Inteligência artificial é um software que aprende padrões a partir de exemplos, em vez de seguir só regras escritas à mão." },
        { "Terminal", "A NÓVA é uma IA generalista: atende vários serviços da cidade. Isso não a torna infalível." } } },
    /* 2 */ { 2, {
        { "Terminal", "Dados são os exemplos que a IA usa: números, textos, registros. Se faltam ou estão errados, o resultado também fica errado." },
        { "Terminal", "Importa quem foi medido, quem ficou de fora e como cada exemplo foi rotulado." } } },
    /* 3 */ { 2, {
        { "Terminal", "Treinar um modelo é ajustar seus parâmetros para errar menos nos exemplos. Depois ele é testado com exemplos novos." },
        { "Terminal", "Um modelo que só decora o treino falha no mundo real. Isso se chama overfitting." } } },
    /* 4 */ { 1, {
        { "Placa", "Segure DASH (X ou Shift) com uma direção. O dash volta ao tocar o chão ou um cristal." } } },
    /* 5 */ { 1, {
        { "Placa", "Segure AGARRAR (C ou Ctrl) contra a parede para subir. Pule para se lançar para fora. A energia acaba." } } },
    /* 6 */ { 2, {
        { "Supervisão", "O terminal de perguntas é a sua avaliação inicial: dez perguntas, todas certas. Se errar, você recomeça com variações." },
        { "Supervisão", "Depois, a porta da cidade se abre. Boa sorte, Decifrador." } } },

    /* ---- Ato 2: Rotina de Chamados ---- */
    /* 7 */ { 2, {
        { "Placa", "Distrito do Trânsito. Os semáforos daqui são controlados pela NÓVA e muita gente reclama de esperas injustas." },
        { "Placa", "Atenda os chamados dos cidadãos. Cada decisão sua mexe na Confiança Pública." } } },
    /* 8 */ { 2, {
        { "Placa", "Distrito do Crédito. As plataformas que tremem são dados incompletos: somem quando você confia demais neles." },
        { "Placa", "Não pare em cima delas. Siga em frente." } } },
    /* 9 */ { 2, {
        { "Placa", "Distrito das Notícias. Algumas plataformas tremeluzem como miragem: parecem reais, mas não sustentam." },
        { "Placa", "Assim são as alucinações de IA: respostas plausíveis, porém inventadas." } } },
    /* 10 */ { 2, {
        { "Placa", "Distrito da Saúde. A NÓVA apoia triagens, mas em decisões críticas as pessoas precisam revisar." },
        { "Placa", "Cuidado com os espinhos: um pequeno erro pode custar caro." } } },

    /* ---- Ato 3: O Escândalo ---- */
    /* 11 */ { 2, {
        { "Registro", "Pedidos de crédito com a mesma renda e o mesmo histórico receberam respostas diferentes conforme o bairro do cadastro." },
        { "Registro", "Evidência 1 reunida." } } },
    /* 12 */ { 2, {
        { "Relatório", "Taxa de aprovação: Bairro Norte, cerca de 6 em cada 10. Bairro Sul, cerca de 3 em cada 10." },
        { "Relatório", "Evidência 2 reunida." } } },
    /* 13 */ { 3, {
        { "Base de treino", "A base histórica veio de decisões humanas antigas, que já tratavam os bairros de forma desigual." },
        { "Base de treino", "A NÓVA aprendeu esse padrão como se fosse regra. Os dados ensinaram o viés." },
        { "Base de treino", "Evidência 3 reunida." } } },
    /* 14 */ { 2, {
        { "Supervisão", "Se isso for verdade, é grave. Reúna as evidências e use o terminal de simulação para ver como os dados produziram o resultado." },
        { "Supervisão", "Depois decida como a cidade vai ficar sabendo." } } },

    /* ---- Ato 4: Desfecho ---- */
    /* 15 */ { 2, {
        { "Supervisão", "A cidade inteira está olhando para o Departamento. O que você decidiu até aqui agora pesa na confiança das pessoas." },
        { "Supervisão", "Faltam só as decisões finais e a última avaliação." } } },
    /* 16 */ { 2, {
        { "Terminal final", "Cada escolha mudou a Confiança Pública. Atravesse a porta para ver o que isso significa para a cidade." },
        { "Terminal final", "Lembre-se: a NÓVA decide rápido. Entendê-la é trabalho de todo mundo." } } },
};

const Dialogue *dialogue_get(int id) {
    if (id < 0 || id >= DIALOGUE_COUNT) return NULL;
    return &DIALOGUES[id];
}
