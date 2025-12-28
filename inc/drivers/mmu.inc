; Команды MMU
extern MMU_CMD_RESET     
extern MMU_CMD_GET_TOTAL 
extern MMU_CMD_GET_FREE  
extern MMU_CMD_RESERVE   
extern MMU_CMD_RELEASE   

; Методы драйвера
extern _mmu_vtable        
extern _mmu_detect        
extern _mmu_install       
extern _mmu_get_info      
extern _mmu_command       
extern _mmu_uninstall     
extern _mmu_set_context   
extern _mmu_get_context   
extern _mmu_map_page      

; Методы менеджера памяти
extern _mmu_mem_init      
extern _mmu_alloc_pages   
extern _mmu_free_pages    
extern _mmu_get_stats     
