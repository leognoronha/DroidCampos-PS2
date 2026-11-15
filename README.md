# DroidCampos-PS2 v0.3.0

Enfrente três ondas de inimigos e acesse o registro desbloqueado no terminal.

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

## Créditos

Consulte [os avisos de terceiros](THIRD-PARTY.md).
