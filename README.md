Programación de Sistemas Operativos
===================================

## Documentación ##

Porque somos modernos y queremos ir (casi) contra la corriente, usamos
[Markdown](http://daringfireball.net/projects/markdown/) como formato para la
documentación de los trabajos, y [pandoc](http://johnmacfarlane.net/pandoc/)
para convertirla a PDF.

Para generar el archivo PDF final:

    $ make doc

### Requerimientos ###

  * LaTeX (obvio)
  * Haskell (woooo)
  * pandoc 1.6+

### Pandoc ###

Me encantaría decirles que con sólo hacer `apt-get install pandoc` estarían
listos, pero lamentablemente me encontré con un bug de _locale_ y _encoding_
poniéndo acentos en el informe, y esto ocurre con la versión 1.5.1, que
casualmente es la de los repos del último Ubuntu.

Van a tener que instalarse la **Plataforma de Haskell** y _vivir en el borde!_

    $ sudo apt-get install haskell-platform
    $ cabal update
    $ cabal install -fhighlighting pandoc

Cabal (el empaquetador haskelliano) instala los binarios en $HOME/.cabal,
tienen que agregarlo en su PATH:

    $ echo "PATH=$HOME/.cabal/bin:$PATH" >> $HOME/.bashrc
    $ source $HOME/.bashrc
    $ hash -r

Chequeen que tengan en su PATH el comando `markdown2pdf` (es un wrapper de `pandoc` y `pdflatex`).

**Más facil, corran `doc/install-pandoc.sh` para hacer todo esto...**
