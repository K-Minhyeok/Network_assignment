else if (strcmp(com.command, "ls") == 0)
		{
			File_info files_list[MAX_FILE_NUM];
			int num_file;

			if (read(sock, &num_file, sizeof(num_file)) > 0)
			{

				for (int i = 0; i < num_file; i++)
				{

					int ls_read_cnt = 0;

					int total = 0;

					while (total < sizeof(File_info))
					{
						ls_read_cnt = read(sock, ((char *)&files_list[i]) + total, sizeof(File_info) - total);
						if (ls_read_cnt <= 0)
						{
							error_handling("read error");
						}

						total += ls_read_cnt;
					}
				}

				for (int i = 0; i < num_file; i++)
				{
					printf("%d : %s | %d \n", i, files_list[i].file_name, files_list[i].size);
				}

				int result;
				if (read(sock, &result, sizeof(result))<0)
				{
					error_handling("result error");
				}
					printf("result : %d\n", result);

			}
			
		}