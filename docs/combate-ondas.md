# Combate por ondas no interior ETIP

O primeiro acesso ao terminal arma o encontro. Fechar o terminal registra o
checkpoint e inicia uma contagem de três segundos. As ondas têm 2, 3 e 5 inimigos;
as duas primeiras concedem até 25 pontos de vida e cinco segundos de intervalo.
Novo jogo e reset de debug apagam o encontro. O progresso não é salvo em disco.

## Comportamento e balanceamento

Os únicos estados de comportamento são alerta (0,8 s), perseguição e combate.
Morte e ausência de spawn são condições de existência do ator, não estados da IA.
O alvo é sempre conhecido; disparos exigem linha de tiro desobstruída.
Até dois inimigos recebem autorização de tiro, alternada a cada dois segundos.
Os demais buscam posições separadas ao redor do jogador.

Os parâmetros estão em `game/inc/core/combat_config.hpp`: jogador com 100 de vida,
inimigo com 30, dano de 10, corrida a 3 m/s, combate entre 6 e 8 m, intervalo de
tiro da IA de 0,8 s e dispersão máxima de 6 graus. Ambos os modelos usam os mesmos
atributos. A arma compartilhada tem oito tiros, reserva infinita, recarga de
aproximadamente 0,514 s e intervalo mínimo de 0,25 s. O jogador recarrega com O;
a IA recarrega automaticamente. Não há headshots, dano de contato ou dano aliado.
Outros personagens bloqueiam o disparo; paredes, vidro e portas também.

O grafo A* é definido para o interior, com desvios junto aos móveis e à cápsula.
Portas fazem parte da rota: o inimigo solicita abertura e aguarda folga física.
O fechamento reverte se qualquer personagem ocupar a passagem. Movimento usa
subpassos conservadores de até 4 cm e deslizamento, com corpos circulares no chão.
Jogador e inimigos bloqueiam a passagem uns dos outros. Os testes de tiro usam
cápsulas verticais e acompanham a altura do jogador ao pular.

Pausa, rádio, terminal, introdução e loading congelam o encontro, armas, portas e
locomoção; o primeiro quadro após retomar descarta o tempo passado fora do jogo.
Ao morrer, X confirma a opção selecionada. Tentar novamente restaura posição,
câmera e portas do checkpoint, vida e carregador completos e reinicia as três
ondas sem exigir outro acesso ao terminal.

## Rádio, terminal e apresentação

O HUD mostra vida, onda, quantidade restante e contagem de preparação/intervalo.
A terceira onda concluída libera uma nova entrada em Ocorrências, mostra o
objetivo de consultar o terminal e transmite:

> PM, A SEGURANÇA FOI MOBILIZADA. DETECTAMOS UMA NOVA ENTRADA NO TERMINAL.
> VOLTE AO TERMINAL E VERIFIQUE.

L2 revisa a mensagem retida. No terminal, selecione Ocorrências e use esquerda ou
direita para alternar entre o registro original e o novo. X abre o registro novo
e conclui o objetivo. O documento é explicitamente provisório; não estabelece a
continuação definitiva da história.

## Recursos e compilação

Cinco malhas possuem buffers de animação independentes. Os 18 quadros e a textura
de cada variante são compartilhados. A carga ocorre em cinco etapas, sem alocar
malhas ou ler arquivos durante o spawn. O gerador `tools/pack_enemy_test.py`
exporta também as posições dos canos para `enemy_muzzles.hpp`.

```powershell
./tests/host/run.ps1
./tools/build-etip.ps1
./tools/run-combat.ps1 -ShowWindow
```

O launcher usa o ELF atual em `game/bin` e cria um perfil isolado em
`artifacts/combat/pcsx2-profile`, copiando o INI de validação já existente.
Não encerra instâncias pessoais do emulador nem altera o INI usado como modelo.
O perfil de combate desabilita `ExtraMemory`, limitando o teste aos 32 MiB do
PS2 de varejo; o modelo anterior de validação permitia 128 MiB. Os logs ficam em
`artifacts/combat`. O build sincroniza as fontes removidas para evitar código
antigo no contêiner.

## Validação

Os testes host cobrem impactos próximos e obstruídos, dano, carregadores independentes,
três estados, duas autorizações de tiro, portas, movimento varrido, checkpoint,
pausas, contagens, cura, ondas 2/3/5 e transmissão única. Uma auditoria por flood
fill independente verifica cinco spawns válidos e rotas a partir de 462 posições
alcançáveis no interior. Três ciclos de carga/descarga verificam cinco animações
independentes e somente duas texturas de inimigos. Terminal e HUD têm verificações
dos limites de 512 x 448.

No PCSX2, completar as três ondas, visitar as quatro salas com inimigos em
perseguição, fechar portas na passagem, testar dano/derrota/reinício, rádio,
registro novo e novas partidas. Conferir alinhamento do cano, animações, feixes,
legibilidade e áudio simultâneo. Esses resultados não são substituídos pelo host.

`[COMBAT MEMORY]` registra RAM disponível e texturas quando muda o número de
inimigos. `[COMBAT PERF]` registra FPS do jogo e médias de 120 quadros para
simulação e submissão 3D, permitindo comparar 0, 2, 3 e 5 inimigos. O tempo de
submissão inclui esperas de DMA, mas não representa medição isolada do GS.
As amostras de RAM não medem pico de alocação nem VRAM. Comparar também a velocidade
da emulação e repetir três percursos equivalentes; validar em PS2 real quando
disponível.


### Resultado desta implementação — 29/10/2026

- Suíte de 20 casos host e auditoria de assets: aprovadas. Os casos de IA e
  armas/áudio foram repetidos após os últimos ajustes e passaram.
- Build PS2 integrado: aprovado; ELF atual em `game/bin/droid_campos.elf`.
- PCSX2 v2.9.81 com 32 MiB: menu, HUD, modelos, feixes e mensagem final
  inspecionados. Um ELF temporário com os mesmos sistemas percorreu as ondas
  2/3/5, verificou pausa, derrota/reinício, rádio final e desbloqueio do registro.
- Outro ensaio registrou 50 disparos inimigos, 4 recargas automáticas e dois
  recarregamentos do interior. Ambos terminaram com 17 texturas e 23,377 MiB
  disponíveis, contra 23,381 MiB na referência inicial desse ensaio.
- A sala sem inimigos registrou 59 FPS. Na terceira onda foram observados
  29 FPS. Um ensaio concentrado de cinco inimigos registrou uma amostra de
  19 FPS e depois 29 FPS; não há garantia de 30 FPS constantes.
- Os ensaios usam posicionamento controlado, restauração de vida e baixas
  programadas para percorrer os estados. Eles não demonstram equilíbrio de
  dificuldade, uma vitória por controle humano ou precisão final da mira.
- Permanecem pendentes a partida completa sem script, avaliação auditiva,
  métricas de p95/p99, medição de VRAM/picos de RAM e execução no console real.
