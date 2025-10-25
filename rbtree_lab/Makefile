##############################################################
# .PHONY: 이 타겟들은 실제 파일이 아니므로 항상 실행됩니다.
.PHONY: help build test clean drive

# help: 사용 가능한 모든 명령어를 예쁘게 출력합니다.
help: ## 모든 명령어와 설명을 보여줘요!
# http://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
	@grep -E '^[a-zA-Z0-9_%/-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

# build: 'src' 디렉터리에서 소스 코드를 빌드합니다.
build: ## driver 프로그램을 빌드해요!
	$(MAKE) -C src

# test: 테스트를 대화형으로 빌드, 실행, 정리까지 한 번에!
test: ## 테스트를 하나씩 확인하면서 실행해요!
	@echo "--- 🧪 테스트 빌드를 시작할게요... ---"
	cc -Wall -g -DSENTINEL -Isrc src/rbtree.c test/test-rbtree.c -o ./test/test-rbtree
	@echo "\n--- 🔬 테스트를 시작합니다! ---"
	./test/test-rbtree
	@echo "\n--- 🧹 테스트가 끝나서 뒷정리 중... ---"
	rm -f ./test/test-rbtree
	@echo "--- ✨ 깨끗하게 정리 완료! ---"

# drive: 코드를 빌드하고, 실행한 뒤, 자동으로 정리합니다.
drive: ## 프로그램을 빌드, 실행, 정리까지 한 번에!
	@echo "--- 🚀 프로그램을 빌드할게요... ---"
	$(MAKE) build
	@echo "\n--- ▶️ 프로그램을 실행합니다! ---"
	./src/driver
	@echo "\n--- 🧹 실행이 끝나서 뒷정리 중... ---"
	$(MAKE) clean
	@echo "--- ✨ 깨끗하게 정리 완료! ---"

# clean: 빌드 환경을 정리합니다.
clean: ## 빌드하면서 만들어진 파일들을 청소해요!
	$(MAKE) -C src clean
	rm -f ./test/test-rbtree


# .PHONY: help build test

# help:
# # http://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
# 	@grep -E '^[a-zA-Z0-9_%/-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

# build:
# build: ## Build executables
# 	$(MAKE) -C src

# test:
# test: ## Test rbtree implementation
# 	$(MAKE) -C test test
	
# clean:
# clean: ## Clear build environment
# 	$(MAKE) -C src clean
# 	$(MAKE) -C test clean