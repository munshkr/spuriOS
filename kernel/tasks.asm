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

include_task ut_getpid, "tasks/getpid.pso"
include_task task_kbd, "tasks/task_kbd.pso"
include_task ut_dummy, "tasks/dummy.pso"
include_task task_sin, "tasks/task_sin.pso"
include_task task_pf, "tasks/task_pf.pso"
include_task task_funky, "tasks/task_funky.pso"
include_task ut_cpuid, "tasks/cpuid.pso"
include_task ut_palloc, "tasks/palloc.pso"
