 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdbool.h>
 #include <time.h>
 #include <stddef.h>
 #include <stdint.h>
 #include "list.h"
 #include "hash.h"
 #include "bitmap.h"
 #include "debug.h"
 #include "hex_dump.h"  // hex_dump 함수 선언 포함
 
 /* 상수 정의 */
 #define MAX_OBJECTS 10
 #define MAX_INPUT_LENGTH 1024
 #define TOKEN_LIMIT 20
 
 /* hash_entry 매크로 정의 (구조체 멤버로부터 부모 구조체 주소 계산) */
 #ifndef hash_entry
 #define hash_entry(HASH_ELEM, STRUCT, MEMBER) \
     ((STRUCT *)((uint8_t *)(HASH_ELEM) - offsetof(STRUCT, MEMBER)))
 #endif
 
 /* 전역 객체 배열 */
 struct list *list_arr[MAX_OBJECTS];
 struct hash *hash_arr[MAX_OBJECTS];
 struct bitmap *bmp_arr[MAX_OBJECTS];
 
 /* 사용자 정의 리스트 요소 구조체 */
 struct list_node {
     struct list_elem node_link;  // 리스트 연결 요소
     int value;                  // 저장 데이터
 };
 
 /* 사용자 정의 해시 테이블 요소 구조체 */
 struct hash_node {
     struct list_elem hash_link;  // 해시 테이블 내부 리스트 연결 요소
     int num_value;              // 저장 데이터
 };
 
 /* ---------------------- */
 /*    유틸리티 함수들     */
 /* ---------------------- */
 
 /*
  * extract_index_from_name:
  *   - 객체 이름에서 숫자 부분을 추출하여 인덱스로 변환.
  *   - 객체 이름에 포함된 숫자가 없으면 -1을 반환.
  */
 int extract_index_from_name(const char *obj_name) {
     while (*obj_name && (*obj_name < '0' || *obj_name > '9'))
         obj_name++;
     return (*obj_name) ? atoi(obj_name) : -1;
 }
 
 /*
  * split_line:
  *   - 입력 문자열(inputBuffer)을 공백, 탭, 개행문자를 기준으로 토큰화하여 token_arr 배열에 저장.
  *   - 최대 TOKEN_LIMIT 개의 토큰을 저장하며, 토큰 개수를 반환.
  */
 int split_line(char *inputBuffer, char **token_arr) {
     int token_count = 0;
     char *token = strtok(inputBuffer, " \t\n");
     while (token != NULL && token_count < TOKEN_LIMIT) {
         token_arr[token_count++] = token;
         token = strtok(NULL, " \t\n");
     }
     return token_count;
 }
 
 /* ---------------------- */
 /*   리스트 관련 함수들   */
 /* ---------------------- */
 
 /*
  * compare_list_elements:
  *   - 두 리스트 요소의 value 필드를 비교하는 함수.
  *   - list_insert_ordered, list_sort 등에서 비교 기준으로 사용됨.
  */
 bool compare_list_elements(const struct list_elem *node_a, const struct list_elem *node_b, void *aux_data) {
     struct list_node *first_node = list_entry(node_a, struct list_node, node_link);
     struct list_node *second_node = list_entry(node_b, struct list_node, node_link);
     return first_node->value < second_node->value;
 }
 
 /*
  * get_nth_element:
  *   - 리스트의 n번째 요소(0-indexed)를 반환.
  *   - index가 범위를 벗어나면 NULL을 반환.
  */
 struct list_elem *get_nth_element(struct list *lst, int index) {
     int count = 0;
     for (struct list_elem *curr = list_begin(lst); curr != list_end(lst); curr = list_next(curr)) {
         if (count == index)
             return curr;
         count++;
     }
     return NULL;
 }
 
 /*
  * free_list:
  *   - 리스트의 모든 요소를 제거하고 동적 할당된 메모리를 해제.
  */
 void free_list(struct list *lst) {
     while (!list_empty(lst)) {
         struct list_elem *curr = list_pop_front(lst);
         free(list_entry(curr, struct list_node, node_link));
     }
 }
 
 /*
  * randomize_list:
  *   - Fisher-Yates 알고리즘을 사용하여 리스트의 요소들을 랜덤하게 섞음.
  *   - 내부 배열에 임시로 요소들을 저장한 후 재배치함.
  */
 void randomize_list(struct list *lst) {
     if (list_empty(lst))
         return;
     size_t size = list_size(lst);
     srand((unsigned)time(NULL));
     struct list_elem **node_array = malloc(size * sizeof(struct list_elem *));
     if (!node_array)
         return;
     size_t idx = 0;
     for (struct list_elem *curr = list_begin(lst); curr != list_end(lst); curr = list_next(curr))
         node_array[idx++] = curr;
     for (size_t j = size - 1; j > 0; j--) {
         size_t k = rand() % (j + 1);
         struct list_elem *temp = node_array[j];
         node_array[j] = node_array[k];
         node_array[k] = temp;
     }
     while (!list_empty(lst))
         list_pop_front(lst);
     for (idx = 0; idx < size; idx++)
         list_push_back(lst, node_array[idx]);
     free(node_array);
 }
 
 /*
  * swap_list_elements:
  *   - 두 리스트 요소의 value 필드를 교환 (연결 재구성이 아닌 데이터만 swap)
  */
 void swap_list_elements(struct list_elem *node1, struct list_elem *node2) {
     if (node1 == node2)
         return;
     struct list_node *first_node = list_entry(node1, struct list_node, node_link);
     struct list_node *second_node = list_entry(node2, struct list_node, node_link);
     int temp_val = first_node->value;
     first_node->value = second_node->value;
     second_node->value = temp_val;
 }
 
 /* ---------------------- */
 /*   해시 테이블 관련 함수들   */
 /* ---------------------- */
 
 /*
  * compute_hash:
  *   - 해시 요소의 num_value 값을 기반으로 해싱 처리.
  *   - hash_int() 함수 사용.
  */
 unsigned compute_hash(const struct hash_elem *hash_elem_ptr, void *aux_data) {
     struct hash_node *node_ptr = hash_entry(hash_elem_ptr, struct hash_node, hash_link);
     return hash_int(node_ptr->num_value);
 }
 
 /*
  * compare_hash_elements:
  *   - 두 해시 요소의 num_value 값을 비교하는 함수.
  */
 bool compare_hash_elements(const struct hash_elem *hash_elem1, const struct hash_elem *hash_elem2, void *aux_data) {
     struct hash_node *first_hash = hash_entry(hash_elem1, struct hash_node, hash_link);
     struct hash_node *second_hash = hash_entry(hash_elem2, struct hash_node, hash_link);
     return first_hash->num_value < second_hash->num_value;
 }
 
 /*
  * alternate_hash_int:
  *   - 정수를 해싱하는 대안 함수.
  *   - 곱셈과 XOR 연산을 이용하여 해싱 값을 계산.
  */
 unsigned alternate_hash_int(int num) {
     unsigned u = (unsigned)num;
     return (u * 2654435761u) ^ (u >> 16);
 }
 
 /* ---------------------- */
 /*    비트맵 관련 함수들    */
 /* ---------------------- */
 
 /*
  * expand_bitmap:
  *   - 기존 비트맵보다 큰 new_capacity로 비트맵 확장.
  *   - 기존 비트맵의 값을 새로운 비트맵에 복사한 후, 기존 비트맵을 해제.
  */
 struct bitmap *expand_bitmap(struct bitmap *bmp, int new_capacity) {
     if (new_capacity <= bitmap_size(bmp))
         return bmp;
     struct bitmap *new_bmp = bitmap_create(new_capacity);
     if (!new_bmp)
         return NULL;
     size_t current_size = bitmap_size(bmp);
     for (size_t i = 0; i < current_size; i++)
         bitmap_set(new_bmp, i, bitmap_test(bmp, i));
     bitmap_destroy(bmp);
     return new_bmp;
 }
 
 /* ---------------------- */
 /*     기타 추가 함수들    */
 /* ---------------------- */
 
 /*
  * init_hash_table:
  *   - 주어진 이름에 해당하는 인덱스에 해시 테이블을 생성 및 초기화.
  */
 void init_hash_table(const char *table_name) {
     int index = extract_index_from_name(table_name);
     if (index < 0 || index >= MAX_OBJECTS)
         return;
     hash_arr[index] = malloc(sizeof(struct hash));
     if (hash_arr[index] != NULL) {
         hash_init(hash_arr[index], compute_hash, compare_hash_elements, NULL);
     }
 }
 
 /*
  * init_bitmap:
  *   - 주어진 이름과 비트 수로 비트맵을 생성.
  */
 void init_bitmap(const char *bitmap_name, size_t bit_count) {
     int index = extract_index_from_name(bitmap_name);
     if (index < 0 || index >= MAX_OBJECTS)
         return;
     bmp_arr[index] = bitmap_create(bit_count);
 }
 
 /*
  * init_list:
  *   - 주어진 이름에 해당하는 인덱스에 리스트를 생성 및 초기화.
  */
 void init_list(const char *list_name) {
     int index = extract_index_from_name(list_name);
     if (index < 0 || index >= MAX_OBJECTS)
         return;
     list_arr[index] = malloc(sizeof(struct list));
     if (list_arr[index] != NULL) {
         list_init(list_arr[index]);
     }
 }
 
 /*
  * reset_bitmap_array:
  *   - 전역 비트맵 배열의 모든 요소를 NULL로 초기화.
  */
 void reset_bitmap_array() {
     for (int idx = 0; idx < MAX_OBJECTS; idx++) {
         bmp_arr[idx] = NULL;
     }
 }
 
 /*
  * reset_list_array:
  *   - 전역 리스트 배열의 모든 요소를 NULL로 초기화.
  */
 void reset_list_array() {
     for (int idx = 0; idx < MAX_OBJECTS; idx++) {
         list_arr[idx] = NULL;
     }
 }
 
 /*
  * print_bitmap_binary:
  *   - 비트맵의 각 비트를 0 또는 1로 출력.
  */
 void print_bitmap_binary(const struct bitmap *bmp) {
     if (!bmp)
         return;
     for (size_t i = 0; i < bitmap_size(bmp); i++) {
         printf("%d", bitmap_test(bmp, i) ? 1 : 0);
     }
     printf("\n");
     fflush(stdout);
 }
 
 /*
  * print_hash_element:
  *   - 해시 테이블의 각 요소의 데이터를 출력 (hash_apply 내에서 사용).
  */
 static void print_hash_element(struct hash_elem *hashElem, void *aux_data) {
     struct hash_node *node_ptr = hash_entry(hashElem, struct hash_node, hash_link);
     printf("%d ", node_ptr->num_value);
 }
 
 /*
  * print_hash_table:
  *   - 해시 테이블의 모든 요소를 출력.
  */
 void print_hash_table(const struct hash *hashTbl) {
     if (!hashTbl)
         return;
     hash_apply((struct hash *)hashTbl, print_hash_element);
     printf("\n");
     fflush(stdout);
 }
 
 /*
  * square_element:
  *   - 해시 테이블 요소의 num_value 값을 제곱.
  */
 void square_element(struct hash_elem *hashElem, void *aux_data) {
     struct hash_node *node_ptr = hash_entry(hashElem, struct hash_node, hash_link);
     node_ptr->num_value *= node_ptr->num_value;
 }
 
 /*
  * cube_element:
  *   - 해시 테이블 요소의 num_value 값을 세제곱.
  */
 void cube_element(struct hash_elem *hashElem, void *aux_data) {
     struct hash_node *node_ptr = hash_entry(hashElem, struct hash_node, hash_link);
     node_ptr->num_value = node_ptr->num_value * node_ptr->num_value * node_ptr->num_value;
 }
 
 /*
  * find_element_by_value:
  *   - 해시 테이블에서 주어진 search_value와 일치하는 요소를 찾음.
  */
 struct hash_elem *find_element_by_value(struct hash *hashTbl, int search_value) {
     struct hash_node tmp_node;
     tmp_node.num_value = search_value;
     return hash_find(hashTbl, (struct hash_elem *)&tmp_node.hash_link);
 }
 
 /*
  * insert_element_at:
  *   - 리스트의 특정 위치에 새로운 요소를 삽입.
  */
 void insert_element_at(struct list *lst, int position, int value) {
     struct list_node *new_node = malloc(sizeof(struct list_node));
     if (!new_node)
         return;
     new_node->value = value;
     struct list_elem *pos_elem = get_nth_element(lst, position);
     if (!pos_elem)
         pos_elem = list_end(lst);
     list_insert(pos_elem, &new_node->node_link);
 }
 
 /* ---------------------- */
 /*    명령어 처리 함수들    */
 /* ---------------------- */
 
 /*
  * process_create_command:
  *   - "create" 명령어를 처리하여 list, hashtable, bitmap 생성.
  */
 void process_create_command(char **cmd_tokens, int token_count) {
     if (token_count < 3)
         return;
     if (strcmp(cmd_tokens[1], "list") == 0) {
         init_list(cmd_tokens[2]);
     }
     else if (strcmp(cmd_tokens[1], "hashtable") == 0) {
         init_hash_table(cmd_tokens[2]);
     }
     else if (strcmp(cmd_tokens[1], "bitmap") == 0 && token_count >= 4) {
         size_t bit_count = (size_t)atoi(cmd_tokens[3]);
         init_bitmap(cmd_tokens[2], bit_count);
     }
 }
 
 /*
  * process_delete_command:
  *   - "delete" 명령어를 처리하여 해당 인덱스의 리스트, 해시 테이블, 비트맵을 삭제.
  */
 void process_delete_command(char **cmd_tokens, int token_count) {
     if (token_count < 2)
         return;
     int index = extract_index_from_name(cmd_tokens[1]);
     if (index < 0 || index >= MAX_OBJECTS)
         return;
     if (list_arr[index] != NULL) {
         free_list(list_arr[index]);
         free(list_arr[index]);
         list_arr[index] = NULL;
     }
     else if (hash_arr[index] != NULL) {
         hash_destroy(hash_arr[index], NULL);
         free(hash_arr[index]);
         hash_arr[index] = NULL;
     }
     else if (bmp_arr[index] != NULL) {
         bitmap_destroy(bmp_arr[index]);
         bmp_arr[index] = NULL;
     }
 }
 
 /*
  * process_dumpdata_command:
  *   - "dumpdata" 명령어를 처리하여 해당 인덱스의 자료구조 데이터를 출력.
  */
 void process_dumpdata_command(char **cmd_tokens, int token_count) {
     if (token_count < 2)
         return;
     int index = extract_index_from_name(cmd_tokens[1]);
     if (index < 0 || index >= MAX_OBJECTS)
         return;
     if (list_arr[index] != NULL) {
         bool first_output = true;
         for (struct list_elem *curr = list_begin(list_arr[index]); curr != list_end(list_arr[index]); curr = list_next(curr)) {
             struct list_node *node_ptr = list_entry(curr, struct list_node, node_link);
             if (!first_output)
                 printf(" ");
             printf("%d", node_ptr->value);
             first_output = false;
         }
         printf("\n");
     }
     else if (hash_arr[index] != NULL) {
         print_hash_table(hash_arr[index]);
     }
     else if (bmp_arr[index] != NULL) {
         print_bitmap_binary(bmp_arr[index]);
     }
     fflush(stdout);
 }
 
 /*
  * process_hash_command:
  *   - 해시 테이블 관련 명령어 처리.
  *   - hash_apply, hash_clear, hash_delete, hash_empty, hash_find, hash_insert, hash_replace, hash_size 등.
  */
 void process_hash_command(char **cmd_tokens, int token_count) {
     if (token_count < 2)
         return;
     int index = extract_index_from_name(cmd_tokens[1]);
     if (index < 0 || index >= MAX_OBJECTS || hash_arr[index] == NULL)
         return;
     struct hash *hashTbl = hash_arr[index];
     if (strcmp(cmd_tokens[0], "hash_apply") == 0) {
         char op_str[20] = {0};
         if (token_count >= 3) {
             sscanf(cmd_tokens[2], "%19s", op_str);
             if (strcmp(op_str, "square") == 0)
                 hash_apply(hashTbl, square_element);
             else if (strcmp(op_str, "triple") == 0)
                 hash_apply(hashTbl, cube_element);
             else
                 hash_apply(hashTbl, print_hash_element);
         }
         else {
             hash_apply(hashTbl, print_hash_element);
         }
         printf("\n");
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "hash_clear") == 0) {
         hash_clear(hashTbl, NULL);
     }
     else if (strcmp(cmd_tokens[0], "hash_delete") == 0 && token_count >= 3) {
         int value_to_delete = atoi(cmd_tokens[2]);
         struct hash_node tmp_node;
         tmp_node.num_value = value_to_delete;
         hash_delete(hashTbl, (struct hash_elem *)&tmp_node.hash_link);
     }
     else if (strcmp(cmd_tokens[0], "hash_empty") == 0) {
         printf("%s\n", hash_empty(hashTbl) ? "true" : "false");
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "hash_find") == 0 && token_count >= 3) {
         int search_val = atoi(cmd_tokens[2]);
         struct hash_node tmp_node;
         memset(&tmp_node, 0, sizeof(tmp_node));
         tmp_node.num_value = search_val;
         struct hash_elem *found_elem = hash_find(hashTbl, (struct hash_elem *)&tmp_node.hash_link);
         if (found_elem)
             printf("%d\n", hash_entry(found_elem, struct hash_node, hash_link)->num_value);
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "hash_insert") == 0 && token_count >= 3) {
         int insert_val = atoi(cmd_tokens[2]);
         struct hash_node *new_node = malloc(sizeof(struct hash_node));
         new_node->num_value = insert_val;
         hash_insert(hashTbl, (struct hash_elem *)&new_node->hash_link);
     }
     else if (strcmp(cmd_tokens[0], "hash_replace") == 0 && token_count >= 3) {
         int replace_val = atoi(cmd_tokens[2]);
         struct hash_node *new_node = malloc(sizeof(struct hash_node));
         new_node->num_value = replace_val;
         hash_replace(hashTbl, (struct hash_elem *)&new_node->hash_link);
     }
     else if (strcmp(cmd_tokens[0], "hash_size") == 0) {
         printf("%zu\n", hash_size(hashTbl));
         fflush(stdout);
     }
 }
 
 /*
  * process_list_command:
  *   - 리스트 관련 명령어 처리.
  *   - list_front, list_back, list_push_front, list_push_back, list_insert, list_insert_ordered,
  *     list_empty, list_max, list_min, list_reverse, list_shuffle, list_size, list_sort,
  *     list_splice, list_swap, list_remove, list_unique 등.
  */
 void process_list_command(char **cmd_tokens, int token_count) {
     if (token_count < 2)
         return;
     int index = extract_index_from_name(cmd_tokens[1]);
     if (index < 0 || index >= MAX_OBJECTS || list_arr[index] == NULL)
         return;
     struct list *lst = list_arr[index];
 
     if (strcmp(cmd_tokens[0], "list_front") == 0) {
         if (!list_empty(lst)) {
             struct list_node *node_ptr = list_entry(list_front(lst), struct list_node, node_link);
             printf("%d\n", node_ptr->value);
             fflush(stdout);
         }
     }
     else if (strcmp(cmd_tokens[0], "list_back") == 0) {
         if (!list_empty(lst)) {
             struct list_node *node_ptr = list_entry(list_back(lst), struct list_node, node_link);
             printf("%d\n", node_ptr->value);
             fflush(stdout);
         }
     }
     else if (strcmp(cmd_tokens[0], "list_pop_front") == 0) {
         if (!list_empty(lst)) {
             (void)list_pop_front(lst);
             fflush(stdout);
         }
     }
     else if (strcmp(cmd_tokens[0], "list_pop_back") == 0) {
         if (!list_empty(lst)) {
             (void)list_pop_back(lst);
             fflush(stdout);
         }
     }
     else if (strcmp(cmd_tokens[0], "list_push_back") == 0 && token_count >= 3) {
         int new_value = atoi(cmd_tokens[2]);
         struct list_node *new_node = malloc(sizeof(struct list_node));
         new_node->value = new_value;
         list_push_back(lst, &new_node->node_link);
     }
     else if (strcmp(cmd_tokens[0], "list_push_front") == 0 && token_count >= 3) {
         int new_value = atoi(cmd_tokens[2]);
         struct list_node *new_node = malloc(sizeof(struct list_node));
         new_node->value = new_value;
         list_push_front(lst, &new_node->node_link);
     }
     else if (strcmp(cmd_tokens[0], "list_insert") == 0 && token_count >= 4) {
         int pos = atoi(cmd_tokens[2]);
         int new_value = atoi(cmd_tokens[3]);
         insert_element_at(lst, pos, new_value);
     }
     else if (strcmp(cmd_tokens[0], "list_insert_ordered") == 0 && token_count >= 3) {
         int new_value = atoi(cmd_tokens[2]);
         struct list_node *new_node = malloc(sizeof(struct list_node));
         new_node->value = new_value;
         list_insert_ordered(lst, &new_node->node_link, compare_list_elements, NULL);
     }
     else if (strcmp(cmd_tokens[0], "list_empty") == 0) {
         bool is_empty = list_empty(lst);
         printf("%s\n", is_empty ? "true" : "false");
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "list_max") == 0) {
         if (!list_empty(lst)) {
             struct list_node *node_ptr = list_entry(list_max(lst, compare_list_elements, NULL), struct list_node, node_link);
             printf("%d\n", node_ptr->value);
             fflush(stdout);
         }
     }
     else if (strcmp(cmd_tokens[0], "list_min") == 0) {
         if (!list_empty(lst)) {
             struct list_node *node_ptr = list_entry(list_min(lst, compare_list_elements, NULL), struct list_node, node_link);
             printf("%d\n", node_ptr->value);
             fflush(stdout);
         }
     }
     else if (strcmp(cmd_tokens[0], "list_reverse") == 0) {
         list_reverse(lst);
     }
     else if (strcmp(cmd_tokens[0], "list_shuffle") == 0) {
         randomize_list(lst);
     }
     else if (strcmp(cmd_tokens[0], "list_size") == 0) {
         printf("%zu\n", list_size(lst));
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "list_sort") == 0) {
         list_sort(lst, compare_list_elements, NULL);
     }
     else if (strcmp(cmd_tokens[0], "list_splice") == 0 && token_count >= 6) {
         int dest_idx = extract_index_from_name(cmd_tokens[1]);      // 대상 리스트 인덱스
         int dest_position = atoi(cmd_tokens[2]);                      // 대상 리스트 내 삽입 위치
         int src_idx = extract_index_from_name(cmd_tokens[3]);         // 원본 리스트 인덱스
         int src_start = atoi(cmd_tokens[4]);                          // 원본 리스트에서 이동할 첫 요소 위치
         int src_end_exclusive = atoi(cmd_tokens[5]);                  // 원본 리스트에서 이동할 마지막 요소의 다음 위치
 
         if (dest_idx < 0 || dest_idx >= MAX_OBJECTS || list_arr[dest_idx] == NULL ||
             src_idx < 0 || src_idx >= MAX_OBJECTS || list_arr[src_idx] == NULL) {
             printf("Invalid list index.\n");
             return;
         }
         
         struct list *dest_list = list_arr[dest_idx];
         struct list *src_list = list_arr[src_idx];
 
         struct list_elem *dest_pos_elem = get_nth_element(dest_list, dest_position);
         struct list_elem *src_start_elem = get_nth_element(src_list, src_start);
         struct list_elem *src_end_elem = get_nth_element(src_list, src_end_exclusive);
 
         if (!dest_pos_elem || !src_start_elem || !src_end_elem) {
             printf("Invalid position.\n");
             return;
         }
         list_splice(dest_pos_elem, src_start_elem, src_end_elem);
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "list_swap") == 0 && token_count >= 4) {
         int pos1 = atoi(cmd_tokens[2]);
         int pos2 = atoi(cmd_tokens[3]);
         struct list_elem *elem1 = get_nth_element(lst, pos1);
         struct list_elem *elem2 = get_nth_element(lst, pos2);
         if (elem1 && elem2)
             swap_list_elements(elem1, elem2);
     }
     else if (strcmp(cmd_tokens[0], "list_remove") == 0 && token_count >= 3) {
         int pos = atoi(cmd_tokens[2]);
         struct list_elem *elem_to_remove = get_nth_element(lst, pos);
         if (elem_to_remove) {
             list_remove(elem_to_remove);
             free(list_entry(elem_to_remove, struct list_node, node_link));
         }
     }
     else if (strcmp(cmd_tokens[0], "list_unique") == 0) {
         int primary_idx = extract_index_from_name(cmd_tokens[1]);
         if (primary_idx < 0 || primary_idx >= MAX_OBJECTS || list_arr[primary_idx] == NULL) {
             printf("Invalid list index.\n");
             return;
         }
         if (token_count >= 3) {
             int secondary_idx = extract_index_from_name(cmd_tokens[2]);
             if (secondary_idx < 0 || secondary_idx >= MAX_OBJECTS || list_arr[secondary_idx] == NULL) {
                 return;
             }
             list_unique(list_arr[primary_idx], list_arr[secondary_idx], compare_list_elements, NULL);
         } else {
             list_unique(list_arr[primary_idx], NULL, compare_list_elements, NULL);
         }
         fflush(stdout);
     }
 }
 
 /*
  * process_bitmap_command:
  *   - 비트맵 관련 명령어 처리.
  *   - bitmap_all, bitmap_any, bitmap_contains, bitmap_count, bitmap_dump, bitmap_expand, bitmap_flip,
  *     bitmap_mark, bitmap_none, bitmap_reset, bitmap_scan, bitmap_scan_and_flip, bitmap_set, bitmap_set_all,
  *     bitmap_set_multiple, bitmap_size, bitmap_test 등.
  */
 void process_bitmap_command(char **cmd_tokens, int token_count) {
     if (token_count < 2)
         return;
     int index = extract_index_from_name(cmd_tokens[1]);
     if (index < 0 || index >= MAX_OBJECTS || bmp_arr[index] == NULL)
         return;
     struct bitmap *bmp = bmp_arr[index];
 
     if (strcmp(cmd_tokens[0], "bitmap_all") == 0 && token_count >= 4) {
         size_t start_idx = (size_t)atoi(cmd_tokens[2]);
         size_t count_val = (size_t)atoi(cmd_tokens[3]);
         printf("%s\n", bitmap_all(bmp, start_idx, count_val) ? "true" : "false");
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_any") == 0 && token_count >= 4) {
         size_t start_idx = (size_t)atoi(cmd_tokens[2]);
         size_t count_val = (size_t)atoi(cmd_tokens[3]);
         printf("%s\n", bitmap_any(bmp, start_idx, count_val) ? "true" : "false");
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_contains") == 0 && token_count >= 5) {
         size_t start_idx, count_val;
         char boolStr[10];
         if (sscanf(cmd_tokens[2], "%zu", &start_idx) != 1 ||
             sscanf(cmd_tokens[3], "%zu", &count_val) != 1 ||
             sscanf(cmd_tokens[4], "%9s", boolStr) != 1) {
             printf("Invalid command format.\n");
             return;
         }
         bool bool_val;
         if (strcmp(boolStr, "true") == 0)
             bool_val = true;
         else if (strcmp(boolStr, "false") == 0)
             bool_val = false;
         else {
             printf("Invalid value. Please enter 'true' or 'false'.\n");
             return;
         }
         printf("%s\n", bitmap_contains(bmp, start_idx, count_val, bool_val) ? "true" : "false");
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_count") == 0 && token_count >= 5) {
         size_t start_idx = (size_t)atoi(cmd_tokens[2]);
         size_t count_val = (size_t)atoi(cmd_tokens[3]);
         bool bool_val = (strcmp(cmd_tokens[4], "true") == 0);
         printf("%zu\n", bitmap_count(bmp, start_idx, count_val, bool_val));
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_dump") == 0) {
         bitmap_dump(bmp);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_expand") == 0 && token_count >= 3) {
         int additional = atoi(cmd_tokens[2]);
         int new_capacity = (int)bitmap_size(bmp) + additional;
         struct bitmap *new_bmp = expand_bitmap(bmp, new_capacity);
         bmp_arr[index] = new_bmp;
     }
     else if (strcmp(cmd_tokens[0], "bitmap_flip") == 0 && token_count >= 3) {
         size_t bit_index = (size_t)atoi(cmd_tokens[2]);
         bitmap_flip(bmp, bit_index);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_mark") == 0 && token_count >= 3) {
         size_t bit_index = (size_t)atoi(cmd_tokens[2]);
         bitmap_mark(bmp, bit_index);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_none") == 0 && token_count >= 4) {
         size_t start_idx = (size_t)atoi(cmd_tokens[2]);
         size_t count_val = (size_t)atoi(cmd_tokens[3]);
         printf("%s\n", bitmap_none(bmp, start_idx, count_val) ? "true" : "false");
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_reset") == 0 && token_count >= 3) {
         size_t bit_index = (size_t)atoi(cmd_tokens[2]);
         bitmap_reset(bmp, bit_index);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_scan") == 0 && token_count >= 5) {
         size_t start_idx, count_val;
         char boolStr[10];
         if (sscanf(cmd_tokens[2], "%zu", &start_idx) != 1 ||
             sscanf(cmd_tokens[3], "%zu", &count_val) != 1 ||
             sscanf(cmd_tokens[4], "%9s", boolStr) != 1) {
             printf("Invalid command format.\n");
             return;
         }
         bool bool_val;
         if (strcmp(boolStr, "true") == 0)
             bool_val = true;
         else if (strcmp(boolStr, "false") == 0)
             bool_val = false;
         else {
             printf("Invalid value. Please enter 'true' or 'false'.\n");
             return;
         }
         size_t index_found = bitmap_scan(bmp, start_idx, count_val, bool_val);
         if (index_found == BITMAP_ERROR)
             printf("%llu\n", (unsigned long long)BITMAP_ERROR);
         else
             printf("%zu\n", index_found);
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_scan_and_flip") == 0 && token_count >= 5) {
         size_t start_idx, count_val;
         char boolStr[10];
         if (sscanf(cmd_tokens[2], "%zu", &start_idx) != 1 ||
             sscanf(cmd_tokens[3], "%zu", &count_val) != 1 ||
             sscanf(cmd_tokens[4], "%9s", boolStr) != 1) {
             printf("Invalid command format.\n");
             return;
         }
         bool bool_val;
         if (strcmp(boolStr, "true") == 0)
             bool_val = true;
         else if (strcmp(boolStr, "false") == 0)
             bool_val = false;
         else {
             printf("Invalid value. Please enter 'true' or 'false'.\n");
             return;
         }
         size_t index_found = bitmap_scan_and_flip(bmp, start_idx, count_val, bool_val);
         if (index_found == BITMAP_ERROR)
             printf("%llu\n", (unsigned long long)BITMAP_ERROR);
         else
             printf("%zu\n", index_found);
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_set") == 0 && token_count >= 4) {
         size_t bit_index = (size_t)atoi(cmd_tokens[2]);
         bool bool_val = (strcmp(cmd_tokens[3], "true") == 0);
         bitmap_set(bmp, bit_index, bool_val);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_set_all") == 0 && token_count >= 3) {
         bool bool_val = (strcmp(cmd_tokens[2], "true") == 0);
         bitmap_set_all(bmp, bool_val);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_set_multiple") == 0 && token_count >= 5) {
         size_t start_idx = (size_t)atoi(cmd_tokens[2]);
         size_t count_val = (size_t)atoi(cmd_tokens[3]);
         bool bool_val = (strcmp(cmd_tokens[4], "true") == 0);
         bitmap_set_multiple(bmp, start_idx, count_val, bool_val);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_size") == 0) {
         printf("%zu\n", bitmap_size(bmp));
         fflush(stdout);
     }
     else if (strcmp(cmd_tokens[0], "bitmap_test") == 0 && token_count >= 3) {
         size_t bit_index = (size_t)atoi(cmd_tokens[2]);
         printf("%s\n", bitmap_test(bmp, bit_index) ? "true" : "false");
         fflush(stdout);
     }
 }
 
 /* ---------------------- */
 /*          main         */
 /* ---------------------- */
 
 int main(void) {
     /* 전역 객체 배열 초기화 */
     for (int idx = 0; idx < MAX_OBJECTS; idx++) {
         list_arr[idx] = NULL;
         hash_arr[idx] = NULL;
         bmp_arr[idx] = NULL;
     }
 
     char inputBuffer[MAX_INPUT_LENGTH];
     char *cmdTokens[TOKEN_LIMIT];
 
     /* 명령어 입력 처리 루프 */
     while (fgets(inputBuffer, sizeof(inputBuffer), stdin)) {
         int numTokens = split_line(inputBuffer, cmdTokens);
         if (numTokens == 0)
             continue;
         if (strcmp(cmdTokens[0], "quit") == 0)
             break;
         if (strcmp(cmdTokens[0], "create") == 0)
             process_create_command(cmdTokens, numTokens);
         else if (strcmp(cmdTokens[0], "delete") == 0)
             process_delete_command(cmdTokens, numTokens);
         else if (strcmp(cmdTokens[0], "dumpdata") == 0)
             process_dumpdata_command(cmdTokens, numTokens);
         else if (strncmp(cmdTokens[0], "hash_", 5) == 0)
             process_hash_command(cmdTokens, numTokens);
         else if (strncmp(cmdTokens[0], "list_", 5) == 0)
             process_list_command(cmdTokens, numTokens);
         else if (strncmp(cmdTokens[0], "bitmap_", 7) == 0)
             process_bitmap_command(cmdTokens, numTokens);
     }
     return 0;
 }
 