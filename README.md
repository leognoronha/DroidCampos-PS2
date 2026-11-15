# DroidCampos-PS2 v0.4.0

Abra o registro após as ondas para liberar o DroidCampos e iniciar a perseguição.

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

## Créditos

Consulte [os avisos de terceiros](THIRD-PARTY.md).
