.PHONY: clean All

All:
	@echo "----------Building project:[ SpeedFan - Debug ]----------"
	@cd "SpeedFan" && "$(MAKE)" -f  "SpeedFan.mk"
clean:
	@echo "----------Cleaning project:[ SpeedFan - Debug ]----------"
	@cd "SpeedFan" && "$(MAKE)" -f  "SpeedFan.mk" clean
