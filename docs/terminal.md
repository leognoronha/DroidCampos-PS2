# Terminal de arquivos ETIP

Interface 512 x 448, fundo preto, tinta ciano esverdeada e molduras blueprint
com cantos recortados, usando o drawer e a fonte do menu. Scanlines respeitam
a opcao de video. Nao carrega texturas ou fontes adicionais.

Tres topicos fixos a esquerda: Corporativo, Historico e Ocorrencias. O painel
direito mostra uma previa ou um documento de duas paginas. Os textos sao
placeholders explicitos, sem estabelecer fatos da historia. Os textos e suas
quebras de linha ficam em `HudSystem::renderTerminal`, em `hud_system.cpp`.
Ao substituir o conteudo, manter as linhas dentro do painel de leitura.

## Controles

- Perto do terminal no interior, Triangulo abre a interface.
- Uma inicializacao de 24 atualizacoes antecede a navegacao.
- Cima/baixo seleciona um dos tres topicos, com retorno circular.
- X abre o documento; esquerda/direita troca paginas, sem ultrapassar os limites.
- Circulo volta da leitura para a lista; na lista, fecha a interface.
- Triangulo fecha em qualquer momento, inclusive durante a inicializacao.
- Analogico esquerdo tambem navega, com a repeticao usada pelo menu.

Cada acesso comeca no primeiro topico. Confirmacao, navegacao e retorno usam
os eventos de audio existentes do menu. Movimento, camera, pulo, tiro, recarga,
radio, pausa e atalhos de debug nao recebem input durante o acesso. O quadro
de fechamento tambem consome os comandos, evitando recarga ou tiro acidental.
O controlador do jogador congela posicao e camera enquanto o terminal esta
aberto; a arma cancela seus efeitos temporarios na entrada. A saida volta ao
gameplay na mesma vista. Nao ha persistencia de leitura entre acessos.

## Validacao

`tests/host/terminal_test.cpp`, incluido em `tests/host/run.ps1`, cobre entrada,
inicializacao, tres topicos, paginas e limites, saida durante inicializacao,
propriedade do input, congelamento de camera/posicao, limites das submissões
de sprites em 512 x 448, ocultacao do HUD e liberacao dos recursos do drawer.

A validacao host nao substitui revisao de legibilidade, audio, scanlines e
temporizacao em PCSX2 ou hardware PS2.

Validacao em 2026-10-29: suite host completa e auditoria de assets passaram;
`make build-engine all` no container tyra-compiler concluiu e o ELF foi copiado
para `game/bin/droid_campos.elf`. Revisao visual/interativa em PCSX2 pendente.
