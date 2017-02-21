MESAGRAMMARBUILDDIR=MesaGrammarBuildDir

  MESA_GRAMMAR_SRCS =	$(MESAGRAMMARBUILDDIR)grammar_mesa.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(grammar.c, $(MESASRCDIR)/src/mesa/shader/grammar)
LinkSourceFile(grammar.h, $(MESASRCDIR)/src/mesa/shader/grammar)
LinkSourceFile(grammar_syn.h, $(MESASRCDIR)/src/mesa/shader/grammar)
LinkSourceFile(grammar_mesa.c, $(MESASRCDIR)/src/mesa/shader/grammar)
LinkSourceFile(grammar_mesa.h, $(MESASRCDIR)/src/mesa/shader/grammar)
#endif

  MESA_GRAMMAR_OBJS =	 $(MESAGRAMMARBUILDDIR)grammar_mesa.o
