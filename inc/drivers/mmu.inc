; Команды MMU
extern MMU_CMD_RESET     
extern MMU_CMD_GET_TOTAL 
extern MMU_CMD_GET_FREE  
extern MMU_CMD_RESERVE   
extern MMU_CMD_RELEASE   

; Методы драйвера
extern __mmu_vtable        
extern __mmu_detect        
extern __mmu_install       
extern __mmu_get_info      
extern __mmu_command       
extern __mmu_uninstall     
extern __mmu_set_context   
extern __mmu_get_context   
extern __mmu_map_page      

; Методы менеджера памяти
extern __mmu_mem_init      
extern __mmu_alloc_pages   
extern __mmu_free_pages    
extern __mmu_get_stats     
