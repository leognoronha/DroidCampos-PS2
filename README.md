# DroidCampos-PS2 v1.1.0

Lançamento privado do episódio ETIP para PlayStation 2.

## Compilar

Com o Docker Desktop em execução, rode `docker compose run --rm compiler`
na raiz. O build gera `build/ps2/droid_campos.elf` e os recursos ao lado do
executável. No PCSX2, abra esse ELF com a BIOS configurada.

## Controles

- Analógico esquerdo para mover e direito para mirar.
- Quadrado para interagir com o cenário.
- X para pular, L1 para correr, R1 para atirar e Círculo para recarregar.
- Círculo fecha as transmissões automáticas.
- Triângulo abre o terminal quando estiver próximo.
- Start abre a pausa. L2 alterna a revisão do rádio e R2 usa um kit médico.
- Pegue a pistola e o cartão na guarita antes de usar o leitor externo.

## PlayStation 2 por USB

Extraia o pacote em um pendrive FAT32 e abra `DroidCampos` na lista **Apps**
do OPL. O ELF, o `title.cfg` e os recursos ficam em `APPS/DroidCampos/`.

O primeiro checkpoint é salvo ao entrar no interior da ETIP. O jogo atualiza
o save ao iniciar as ondas e após a terceira onda. **Load Game** retoma o
último checkpoint válido no Memory Card da entrada 1. O jogo também funciona
sem cartão, com o retry das ondas em memória.

## Créditos

Consulte [os avisos de terceiros](THIRD-PARTY.md).
