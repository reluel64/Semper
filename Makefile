.PHONY: clean All

All:
	@echo "----------Building project:[ Semper - Debug ]----------"
	@cd "Semper" && "$(MAKE)" -f  "Semper.mk" PreBuild && "$(MAKE)" -f  "Semper.mk" && "$(MAKE)" -f  "Semper.mk" PostBuild
clean:
	@echo "----------Cleaning project:[ Semper - Debug ]----------"
	@cd "Semper" && "$(MAKE)" -f  "Semper.mk" clean
