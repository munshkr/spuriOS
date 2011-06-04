;
; Tasks.asm, truco para incluir las tareas en el kernel.bin
;


%macro include_task 2 ; task , taskfile
global task_%1_pso
task_%1_pso:
incbin %2
global task_%1_pso_end
task_%1_pso_end:
%endmacro

include_task init, "tasks/init.pso"
