REFPKGS  = cmd
SRCPKGS  = mem util
LIBPKGS  = $(REFPKGS) $(SRCPKGS)
MAIN     = main

LIBS     = $(addprefix -l, $(LIBPKGS))
SRCLIBS  = $(addsuffix .a, $(addprefix lib, $(SRCPKGS)))

EXEC     = memTest

.PHONY : all debug

all:   EXEC  = memTest
debug: EXEC  = memTest.debug

all:  DEBUG_FLAG =
debug:DEBUG_FLAG = -DMEM_DEBUG

all debug: libs main

libs:
	@for pkg in $(SRCPKGS); \
	do \
		echo "Checking $$pkg..."; \
		cd src/$$pkg; make --no-print-directory \
                                   DEBUG_FLAG=$(DEBUG_FLAG) PKGNAME=$$pkg; \
		cd ../..; \
	done

main:
	@echo "Checking $(MAIN)..."
	@cd src/$(MAIN);  \
            make --no-print-directory INCLIB="$(LIBS)" EXEC=$(EXEC);
	@ln -fs bin/$(EXEC) .
#	@strip bin/$(EXEC)

clean:
	@for pkg in $(SRCPKGS); \
	do \
		echo "Cleaning $$pkg..."; \
		cd src/$$pkg; make --no-print-directory PKGNAME=$$pkg clean; \
                cd ../..; \
	done
	@echo "Cleaning $(MAIN)..."
	@cd src/$(MAIN); make --no-print-directory clean
	@echo "Removing $(SRCLIBS)..."
	@cd lib; rm -f $(SRCLIBS)
	@echo "Removing $(EXEC)..."
	@rm -f bin/$(EXEC)* 

ctags:	  
	@rm -f src/tags
	@for pkg in $(SRCPKGS); \
	do \
		echo "Tagging $$pkg..."; \
		cd src; ctags -a $$pkg/*.cpp $$pkg/*.h; cd ..; \
	done
	@echo "Tagging $(MAIN)..."
	@cd src; ctags -a $(MAIN)/*.cpp

32 64 mac:
	@for pkg in $(REFPKGS); \
	do \
		cd lib; ln -sf lib$$pkg-$@.a lib$$pkg.a; cd ../..; \
        done
	@cd ref; ln -sf memTest-$@ memTest; \
		 ln -sf memTest-$@.debug memTest.debug; cd ../..;

