#!/bin/bash

# install Haskell platform (including Cabal package manager)
sudo apt-get install haskell-platform

# install latest pandoc
cabal update
cabal install -fhighlighting pandoc

# put Cabal's executable to path
echo "PATH=$HOME/.cabal/bin:$PATH" >> $HOME/.bashrc
source $HOME/.bashrc

# reset Bash's path cache
hash -r
