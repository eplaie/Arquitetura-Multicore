#include "../policies/policy.h"
#include <stdio.h>
#include "../os_display.h"

Policy* select_scheduling_policy(void) {
    show_policy_menu();
    int choice;
    scanf("%d", &choice);

    Policy* policy = NULL;
    switch (choice) {
        case 1:
            policy = create_rr_policy();
            break;
        case 2:
            policy = create_sjf_policy();
            break;
        case 3:
            policy = create_lottery_policy();
            break;
        case 4:  // Add this case
            policy = create_cache_aware_policy();
            break;
        default:
            printf("\n[Política] ERRO: Opção inválida");
            exit(1);
    }

    if (!policy) {
        printf("\n[Política] ERRO: Falha ao criar política");
        exit(1);
    }

    show_policy_selected(policy->name);
    return policy;
}