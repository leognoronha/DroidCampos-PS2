# Validação restante no PCSX2

As cinco etapas de código estão implementadas. Há 12 testes host aprovados e
build PS2 integrada aprovada. O teste de inicialização no PCSX2 v2.9.81 carregou
engine, áudio, controle, fonte, interface e arma e chegou a `GameApp initialized`.
Isso não comprova pixels do menu/cenário, geometria em movimento, DMA/GS ou FPS.

## Abrir o executável já compilado

Com o PCSX2 fechado, execute na raiz do projeto:

```powershell
./tools/run-optimized.ps1 -ShowWindow
```

Não usa Docker nem recompila. O executável é `artifacts/optimization/hud-step5.elf`;
os recursos estão em `artifacts/optimization/run-integrated/`. O script usa uma
configuração isolada em `artifacts/optimization/pcsx2-validation/PCSX2/`, sem
alterar os INIs ou memory cards pessoais. Ela referencia a BIOS já instalada
localmente, mantém resolução nativa, ciclos EE padrão e velocidade nominal 1,
com cheats e patches desabilitados. Os controles foram copiados da configuração
existente. Cada execução gera seu próprio log em `artifacts/optimization/`.

## O que ainda precisa ser verificado

- Menu e HUD: texto, cores, escalas, quebras de linha e sobreposição.
- Exterior: entrar em novo jogo, aproximar-se das paredes, girar 360 graus,
  caminhar e correr; conferir névoa, arma e acesso pelo leitor.
- Interior: atravessar as quatro portas, visitar terminal e cápsula, conferir
  vidro/profundidade e aproximar-se das paredes durante giros.
- Loading e troca de cena: ausência de travamentos ou texturas corrompidas.
- Desempenho: mesmas rotas, posições, orientação e configuração na referência
  e na versão otimizada; três capturas de 30 segundos após aquecimento. Registrar
  FPS do jogo e velocidade da emulação separadamente. O valor de frame rate de
  inicialização no log do PCSX2 é o alvo do vídeo, não uma medição do jogo.

Para mediana, p95, p99 e pior quadro, são necessárias amostras reais de tempo
por quadro. A instrumentação opcional do plano não foi implementada. Não há
captura comparativa que comprove a meta de 30 FPS/p95 de 33,3 ms. Os ELFs das
etapas 1–5 e seus hashes estão em `artifacts/optimization/elf-manifest.json`;
a comparação histórica completa dos commits do plano continua pendente.

O serviço de controle de janelas falhou com `Computer Use native pipe is
unavailable` (erro 2). Para concluir a validação automática, ele precisa voltar
a funcionar; alternativamente, a validação interativa pode ser feita pelo
usuário. Nenhum resultado visual ou temporal deve ser marcado como aprovado
apenas a partir dos testes host ou do log de inicialização.
