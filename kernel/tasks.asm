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

include_task ut_cp2user, "tasks/cp2user.pso"
include_task ut_getpid, "tasks/getpid.pso"
include_task ut_dummy, "tasks/dummy.pso"
;include_task task_sin, "tasks/task_sin.pso"
;include_task task_pf, "tasks/task_pf.pso"
;include_task task_funky, "tasks/task_funky.pso"
include_task ut_cpuid, "tasks/cpuid.pso"
include_task ut_palloc, "tasks/palloc.pso"
include_task ut_con, "tasks/ut_con.pso"
include_task serial, "tasks/serial.pso"

include_task console, "tasks/console.pso"
include_task polly_test_task, "tasks/polly_test_task.pso"

include_task screen_saver_task, "tasks/screen_saver.pso"
