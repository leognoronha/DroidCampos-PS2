# Inimigos no interior ETIP

A sequência visual de teste foi substituída pela IA e pelo encontro de três
ondas. Consulte [Combate por ondas](combate-ondas.md) para comportamento,
balanceamento, controles, recursos, compilação e roteiro de validação.

Os modelos continuam sendo os dois Cerberus de `animations-v3-fit`, com
18 quadros por variante. A atribuição e a licença permanecem em
`game/res/enemies/SOURCE-LICENSE.md`. Os bancos de posições somam cerca de
2,2 MiB; cinco instâncias compartilham esses bancos e duas texturas, mantendo
buffers de vértices independentes. RAM, VRAM e desempenho precisam ser
avaliados na execução do ELF atual.
