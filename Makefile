.PHONY: sender receiver-ros2 run-receiver launch clean

# 1. sender のみのビルド
sender:
	$(MAKE) -C controller_sender

# 2. receiver と ros2 のビルド
receiver-ros2:
	$(MAKE) -C controller_receiver
	$(MAKE) -C ros2

# 3. 単体 receiver の起動
run-receiver:
	$(MAKE) -C controller_receiver run

# 4. ros2 node 起動 (make launch)
launch: receiver-ros2
	$(MAKE) -C ros2 launch

# 5. 全クリーン
clean:
	$(MAKE) -C controller_sender clean
	$(MAKE) -C controller_receiver clean
	$(MAKE) -C ros2 clean
