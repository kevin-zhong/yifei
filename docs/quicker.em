macro AutoExpand()
{
    //������Ϣ
    // get window, sel, and buffer handles
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    sel = GetWndSel(hwnd)
    if(sel.lnFirst != sel.lnLast) 
    {
        /*�������*/
        BlockCommandProc()
    }
    if (sel.ichFirst == 0)
        stop
    hbuf = GetWndBuf(hwnd)
    language = getreg(LANGUAGE)
    if(language != 1)
    {
        language = 0
    }
    nVer = 0
    nVer = GetVersion()
    /*ȡ���û���*/
    szMyName = getreg(MYNAME)
    if(strlen( szMyName ) == 0)
    {
        szMyName = Ask("Enter your name:")
        setreg(MYNAME, szMyName)
    }
    // get line the selection (insertion point) is on
    szLine = GetBufLine(hbuf, sel.lnFirst);
    // parse word just to the left of the insertion point
    wordinfo = GetWordLeftOfIch(sel.ichFirst, szLine)
    ln = sel.lnFirst;
    chTab = CharFromAscii(9)
        
    // prepare a new indented blank line to be inserted.
    // keep white space on left and add a tab to indent.
    // this preserves the indentation level.
    chSpace = CharFromAscii(32);
    ich = 0
    while (szLine[ich] == chSpace || szLine[ich] == chTab)
    {
        ich = ich + 1
    }
    szLine1 = strmid(szLine,0,ich)
    szLine = strmid(szLine, 0, ich) # TabSpace(1)
    
    sel.lnFirst = sel.lnLast
    sel.ichFirst = wordinfo.ich
    sel.ichLim = wordinfo.ich

    /*�Զ���ɼ������ƥ����ʾ*/
    wordinfo.szWord = RestoreCommand(hbuf,wordinfo.szWord)
    sel = GetWndSel(hwnd)
    if (wordinfo.szWord == "pn") /*���ⵥ�ŵĴ���*/
    {
        DelBufLine(hbuf, ln)
        AddPromblemNo()
        return
    }
    /*��������ִ��*/
    else if (wordinfo.szWord == "config" || wordinfo.szWord == "co")
    {
        DelBufLine(hbuf, ln)
        ConfigureSystem()
        return
    }
    /*�޸���ʷ��¼����*/
    else if (wordinfo.szWord == "hi")
    {
        DelBufLine(hbuf, ln)
        InsertHistory(hbuf,ln,language)
        return
    }
    else if (wordinfo.szWord == "abg")
    {
        sel.ichFirst = sel.ichFirst - 3
        SetWndSel(hwnd,sel)
        InsertReviseAdd()
        PutBufLine(hbuf, ln+1 ,szLine1)
        SetBufIns(hwnd,ln+1,sel.ichFirst)
        return
    }
    else if (wordinfo.szWord == "dbg")
    {
        sel.ichFirst = sel.ichFirst - 3
        SetWndSel(hwnd,sel)
        InsertReviseDel()
        PutBufLine(hbuf, ln+1 ,szLine1)
        SetBufIns(hwnd,ln+1,sel.ichFirst)
        return
    }
    else if (wordinfo.szWord == "mbg")
    {
        sel.ichFirst = sel.ichFirst - 3
        SetWndSel(hwnd,sel)
        InsertReviseMod()
        PutBufLine(hbuf, ln+1 ,szLine1)
        SetBufIns(hwnd,ln+1,sel.ichFirst)
        return
    }
    if(language == 1)
    {
        ExpandProcEN(szMyName,wordinfo,szLine,szLine1,nVer,ln,sel)
    }
    else
    {
        ExpandProcCN(szMyName,wordinfo,szLine,szLine1,nVer,ln,sel)
    }
}

macro ExpandProcEN(szMyName,wordinfo,szLine,szLine1,nVer,ln,sel)
{
  
    szCmd = wordinfo.szWord
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    hbuf = GetWndBuf(hwnd)
    /*Ӣ��ע��*/
    if (szCmd == "/*")
    {   
        if(wordinfo.ichLim > 70)
        {
            Msg("The right margine is small, Please use a new line")
            stop 
        }
        szCurLine = GetBufLine(hbuf, sel.lnFirst);
        szLeft = strmid(szCurLine,0,wordinfo.ichLim)
        lineLen = strlen(szCurLine)
        kk = 0
        while(wordinfo.ichLim + kk < lineLen)
        {
            if((szCurLine[wordinfo.ichLim + kk] != " ")||(szCurLine[wordinfo.ichLim + kk] != "\t")
            {
                msg("you must insert /* at the end of a line");
                return
            }
            kk = kk + 1
        }
        szContent = Ask("Please input comment")
        DelBufLine(hbuf, ln)
        szLeft = cat( szLeft, " ")
        CommentContent(hbuf,ln,szLeft,szContent,1)            
        return
    }
    else if(szCmd == "{")
    {
        InsBufLine(hbuf, ln + 1, "@szLine@")
        InsBufLine(hbuf, ln + 2, "@szLine1@" # "}");
        SetBufIns (hbuf, ln + 1, strlen(szLine))
        return
    }
    else if (szCmd == "while" )
    {
        SetBufSelText(hbuf, " ( # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#");
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}");
    }
    else if( szCmd == "else" )
    {
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 2, "@szLine@");
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}");
        SetBufIns (hbuf, ln + 2, strlen(szLine))
        return
    }
    else if (szCmd == "#ifd" || szCmd == "#ifdef") //#ifdef
    {
        DelBufLine(hbuf, ln)
        InsIfdef()
        return
    }
    else if (szCmd == "#ifn" || szCmd == "#ifndef") //#ifndef
    {
        DelBufLine(hbuf, ln)
        InsIfndef()
        return
    }
    else if (szCmd == "#if")
    {
        DelBufLine(hbuf, ln)
        InsertPredefIf()
        return
    }
    else if (szCmd == "cpp")
    {
        DelBufLine(hbuf, ln)
        InsertCPP(hbuf,ln)
        return
    }    
    else if (szCmd == "if")
    {
        SetBufSelText(hbuf, " ( # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#");
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}");
/*            InsBufLine(hbuf, ln + 4, "@szLine1@" # "else");
        InsBufLine(hbuf, ln + 5, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 6, "@szLine@" # ";");
        InsBufLine(hbuf, ln + 7, "@szLine1@" # "}");*/
    }
    else if (szCmd == "ef")
    {
        PutBufLine(hbuf, ln, szLine1 # "else if ( # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#");
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}");
    }
    else if (szCmd == "ife")
    {
        PutBufLine(hbuf, ln, szLine1 # "if ( # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#");
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}");
        InsBufLine(hbuf, ln + 4, "@szLine1@" # "else");
        InsBufLine(hbuf, ln + 5, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 6, "@szLine@" # ";");
        InsBufLine(hbuf, ln + 7, "@szLine1@" # "}");
    }
    else if (szCmd == "ifs")
    {
        PutBufLine(hbuf, ln, szLine1 # "if ( # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#");
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}");
        InsBufLine(hbuf, ln + 4, "@szLine1@" # "else if ( # )");
        InsBufLine(hbuf, ln + 5, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 6, "@szLine@" # ";");
        InsBufLine(hbuf, ln + 7, "@szLine1@" # "}");
        InsBufLine(hbuf, ln + 8, "@szLine1@" # "else");
        InsBufLine(hbuf, ln + 9, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 10, "@szLine@" # ";");
        InsBufLine(hbuf, ln + 11, "@szLine1@" # "}");
    }
    else if (szCmd == "for")
    {
        SetBufSelText(hbuf, " ( # ; # ; # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{")
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#")
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}")
        SetWndSel(hwnd, sel)
        SearchForward()
        szVar = ask("Please input loop variable")
        newsel = sel
        newsel.ichLim = GetBufLineLength (hbuf, ln)
        SetWndSel(hwnd, newsel)
        SetBufSelText(hbuf, " ( @szVar@ = # ; @szVar@ # ; @szVar@++ )")
    }
    else if (szCmd == "fo")
    {
        SetBufSelText(hbuf, "r ( ulI = 0; ulI < # ; ulI++ )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{")
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#")
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}")
        symname =GetCurSymbol ()
        symbol = GetSymbolLocation(symname)
        if(strlen(symbol) > 0)
        {
            nIdx = symbol.lnName + 1;
            while( 1 )
            {
                szCurLine = GetBufLine(hbuf, nIdx);
                nRet = strstr(szCurLine,"{")
                if( nRet != 0xffffffff )
                {
                    break;
                }
                nIdx = nIdx + 1
                if(nIdx > symbol.lnLim)
                {
                    break
                }
             }
             InsBufLine(hbuf, nIdx + 1, "    yf_u32_t ulI = 0;");        
         }
    }
    else if (szCmd == "switch" )
    {
        nSwitch = ask("Please input the number of case")
        SetBufSelText(hbuf, " ( # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{")
        InsertMultiCaseProc(hbuf,szLine1,nSwitch)
    }
    else if (szCmd == "do")
    {
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{")
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#");
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "} while ( # );")
    }
    else if (szCmd == "case" )
    {
        SetBufSelText(hbuf, " # :")
        InsBufLine(hbuf, ln + 1, "@szLine@" # "#")
        InsBufLine(hbuf, ln + 2, "@szLine@" # "break;")
    }
    else if (szCmd == "struct" || szCmd == "st")
    {
        DelBufLine(hbuf, ln)
        szStructName = toupper(Ask("Please input struct name"))
        InsBufLine(hbuf, ln, "@szLine1@typedef struct @szStructName@");
        InsBufLine(hbuf, ln + 1, "@szLine1@{");
        InsBufLine(hbuf, ln + 2, "@szLine@             ");
        szStructName = cat(szStructName,"_STRU")
        InsBufLine(hbuf, ln + 3, "@szLine1@}@szStructName@;");
        SetBufIns (hbuf, ln + 2, strlen(szLine))
        return
    }
    else if (szCmd == "enum" || szCmd == "en")
    {
        DelBufLine(hbuf, ln)
        szStructName = toupper(Ask("Please input enum name"))
        InsBufLine(hbuf, ln, "@szLine1@typedef enum @szStructName@");
        InsBufLine(hbuf, ln + 1, "@szLine1@{");
        InsBufLine(hbuf, ln + 2, "@szLine@             ");
        szStructName = cat(szStructName,"_ENUM")
        InsBufLine(hbuf, ln + 3, "@szLine1@}@szStructName@;");
        SetBufIns (hbuf, ln + 2, strlen(szLine))
        return
    }
    else if (szCmd == "file" || szCmd == "fi")
    {
        DelBufLine(hbuf, ln)
        InsertFileHeaderEN( hbuf,0, szMyName,"" )
        return
    }
    else if (szCmd == "func" || szCmd == "fu")
    {
        DelBufLine(hbuf,ln)
        lnMax = GetBufLineCount(hbuf)
        if(ln != lnMax)
        {
            szNextLine = GetBufLine(hbuf,ln)
            if( (strstr(szNextLine,"(") != 0xffffffff) || (nVer != 2))
            {
                symbol = GetCurSymbol()
                if(strlen(symbol) != 0)
                {  
                    FuncHeadCommentEN(hbuf, ln, symbol, szMyName,0)
                    return
                }
            }
        }
        szFuncName = Ask("Please input function name")
        FuncHeadCommentEN(hbuf, ln, szFuncName, szMyName, 1)
    }
    else if (szCmd == "tab")
    {
        DelBufLine(hbuf, ln)
        ReplaceBufTab()
        return
    }
    else if (szCmd == "ap")
    {   
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day
        
        DelBufLine(hbuf, ln)
        szQuestion = AddPromblemNo()
        InsBufLine(hbuf, ln, "@szLine1@/* Promblem Number: @szQuestion@     Author:@szMyName@,   Date:@sz@/@sz1@/@sz3@ ");
        szContent = Ask("Description")
        szLeft = cat(szLine1,"   Description    : ");
        if(strlen(szLeft) > 70)
        {
            Msg("The right margine is small, Please use a new line")
            stop 
        }
        ln = CommentContent(hbuf,ln + 1,szLeft,szContent,1)
        return
    }
    else if (szCmd == "hd")
    {
        DelBufLine(hbuf, ln)
        CreateFunctionDef(hbuf,szMyName,1)
        return
    }
    else if (szCmd == "hdn")
    {
        DelBufLine(hbuf, ln)

        /*���ɲ�Ҫ�ļ�������ͷ�ļ�*/
        CreateNewHeaderFile()
        return
    }
    else if (szCmd == "ab")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day
        
        DelBufLine(hbuf, ln)
        szQuestion = GetReg ("PNO")
        if(strlen(szQuestion)>0)
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Added by @szMyName@, @sz@/@sz1@/@sz3@   PN:@szQuestion@ */");
        }
        else
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Added by @szMyName@, @sz@/@sz1@/@sz3@ */");        
        }
        return
    }
    else if (szCmd == "ae")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day

        DelBufLine(hbuf, ln)
        InsBufLine(hbuf, ln, "@szLine1@/* END:   Added by @szMyName@, @sz@/@sz1@/@sz3@ */");
        return
    }
    else if (szCmd == "db")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day

        DelBufLine(hbuf, ln)
        szQuestion = GetReg ("PNO")
            if(strlen(szQuestion) > 0)
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Deleted by @szMyName@, @sz@/@sz1@/@sz3@   PN:@szQuestion@ */");
        }
        else
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Deleted by @szMyName@, @sz@/@sz1@/@sz3@ */");
        }
        
        return
    }
    else if (szCmd == "de")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day

        DelBufLine(hbuf, ln + 0)
        InsBufLine(hbuf, ln, "@szLine1@/* END: Deleted by @szMyName@, @sz@/@sz1@/@sz3@ */");
        return
    }
    else if (szCmd == "mb")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day

        DelBufLine(hbuf, ln)
        szQuestion = GetReg ("PNO")
        if(strlen(szQuestion) > 0)
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Modified by @szMyName@, @sz@/@sz1@/@sz3@   PN:@szQuestion@ */");
        }
        else
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Modified by @szMyName@, @sz@/@sz1@/@sz3@ */");
        }
        return
    }
    else if (szCmd == "me")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day

        DelBufLine(hbuf, ln)
        InsBufLine(hbuf, ln, "@szLine1@/* END:   Modified by @szMyName@, @sz@/@sz1@/@sz3@ */");
        return
    }
    else
    {
        SearchForward()
//            ExpandBraceLarge()
        stop
    }
    SetWndSel(hwnd, sel)
    SearchForward()
}


macro ExpandProcCN(szMyName,wordinfo,szLine,szLine1,nVer,ln,sel)
{
    szCmd = wordinfo.szWord
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    hbuf = GetWndBuf(hwnd)

    //����ע��
    if (szCmd == "/*")
    {   
        if(wordinfo.ichLim > 70)
        {
            Msg("�ұ߿ռ�̫С,�����µ���")
            stop 
        }        szCurLine = GetBufLine(hbuf, sel.lnFirst);
        szLeft = strmid(szCurLine,0,wordinfo.ichLim)
        lineLen = strlen(szCurLine)
        kk = 0
        /*ע��ֻ������β������ע�͵����ô���*/
        while(wordinfo.ichLim + kk < lineLen)
        {
            if(szCurLine[wordinfo.ichLim + kk] != " ")
            {
                msg("ֻ������β����");
                return
            }
            kk = kk + 1
        }
        szContent = Ask("������ע�͵�����")
        DelBufLine(hbuf, ln)
        szLeft = cat( szLeft, " ")
        CommentContent(hbuf,ln,szLeft,szContent,1)            
        return
    }
    else if(szCmd == "{")
    {
        InsBufLine(hbuf, ln + 1, "@szLine@")
        InsBufLine(hbuf, ln + 2, "@szLine1@" # "}");
        SetBufIns (hbuf, ln + 1, strlen(szLine))
        return
    }
    else if (szCmd == "while" || szCmd == "wh")
    {
        SetBufSelText(hbuf, " ( # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#");
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}");
    }
    else if( szCmd == "else" || szCmd == "el")
    {
        SetBufSelText(hbuf, " { ")
        //InsBufLine(hbuf, ln + 1, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 1, "@szLine@");
        InsBufLine(hbuf, ln + 2, "@szLine1@" # "}");
        SetBufIns (hbuf, ln + 1, strlen(szLine))
        return
    }
    else if (szCmd == "#ifd" || szCmd == "#ifdef") //#ifdef
    {
        DelBufLine(hbuf, ln)
        InsIfdef()
        return
    }
    else if (szCmd == "#ifn" || szCmd == "#ifndef") //#ifdef
    {
        DelBufLine(hbuf, ln)
        InsIfndef()
        return
    }
    else if (szCmd == "#if")
    {
        DelBufLine(hbuf, ln)
        InsertPredefIf()
        return
    }
    else if (szCmd == "#ifh") //file head
    {
        DelBufLine(hbuf, ln)
        InsFileHead(hbuf)
        return
    }    
    else if (szCmd == "#include")
    {
    	 DelBufLine(hbuf, ln)
        InsIncludeHead(hbuf, ln)
        return
    }
    else if (szCmd == "cpp")
    {
        DelBufLine(hbuf, ln)
        InsertCPP(hbuf,ln)
        return
    }    
    else if (szCmd == "if")
    {
        SetBufSelText(hbuf, " ( # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#");
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}");
/*            InsBufLine(hbuf, ln + 4, "@szLine1@" # "else");
        InsBufLine(hbuf, ln + 5, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 6, "@szLine@" # ";");
        InsBufLine(hbuf, ln + 7, "@szLine1@" # "}");*/
    }
    else if (szCmd == "ef")
    {
        PutBufLine(hbuf, ln, szLine1 # "else if ( # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#");
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}");
    }
    else if (szCmd == "ife")
    {
        PutBufLine(hbuf, ln, szLine1 # "if ( # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#");
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}");
        InsBufLine(hbuf, ln + 4, "@szLine1@" # "else {");
        InsBufLine(hbuf, ln + 5, "@szLine@" # ";");
        InsBufLine(hbuf, ln + 6, "@szLine1@" # "}");
    }
    else if (szCmd == "ifs")
    {
        PutBufLine(hbuf, ln, szLine1 # "if ( # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#");
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}");
        InsBufLine(hbuf, ln + 4, "@szLine1@" # "else if ( # )");
        InsBufLine(hbuf, ln + 5, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 6, "@szLine@" # ";");
        InsBufLine(hbuf, ln + 7, "@szLine1@" # "}");
        InsBufLine(hbuf, ln + 8, "@szLine1@" # "else");
        InsBufLine(hbuf, ln + 9, "@szLine1@" # "{");
        InsBufLine(hbuf, ln + 10, "@szLine@" # ";");
        InsBufLine(hbuf, ln + 11, "@szLine1@" # "}");
    }
    else if (szCmd == "for")
    {
        SetBufSelText(hbuf, " ( # ; # ; # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{")
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#")
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}")
        SetWndSel(hwnd, sel)
        SearchForward()
        szVar = ask("������ѭ������")
        newsel = sel
        newsel.ichLim = GetBufLineLength (hbuf, ln)
        SetWndSel(hwnd, newsel)
        SetBufSelText(hbuf, " ( @szVar@ = # ; @szVar@ # ; @szVar@++ )")
    }
    else if (szCmd == "fo")
    {
        symname =GetCurSymbol ()
        symbol = GetSymbolLocation(symname)
        parent_symbol = MYGetCurSymbol(hbuf, ln)
        //MSG(parent_symbol)

        maxname = 0
	 while (symbol.Symbol != parent_symbol.Symbol)
	 {
	 	leaf_name = SymbolLeafName(parent_symbol)
	 	if (strstr(leaf_name, "for i") != 0xffffffff)
	 	{
	 		//MSG(leaf_name)
	 		vari = strmid(leaf_name, strstr(leaf_name, " ")+1, strstr(leaf_name, "="))
	 		if (strlen(vari) == 2)
	 		{
		 		maxname = vari[1];
		 		//MSG("[" # vari # "] maxname=" # maxname)
		 		break;
	 		}
	 	}
	 	parent_symbol = SymbolParent(parent_symbol)
	 }

	 maxname = maxname + 1;
	 iname = "i@maxname@"
        
    	 //MSG(GetSymbolLocationFromLn(hbuf, ln + 1))
    	 //SetBufSelText(hbuf, "r ( u1 = 0; u1 < # ; u1++ )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{")
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#")
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "}")
        //MSG("line=" # ln)
        //MSG(GetSymbolLocationFromLn(hbuf, ln + 1))
        insert_decal = 0

        if(strlen(symbol) > 0)
        {   
            declare_word = "yf_u32_t @iname@ = 0;"
            //MSG(GetSymbolLocationFromLn(hbuf, symbol.lnName))
	     //Msg (symname # " was found in " # symbol)
	     //MSG("now ln=" # ln)
            nIdx = symbol.lnName;
            insert_line = nIdx

            while (0xffffffff == strstr(GetBufLine(hbuf, insert_line), "{"))
            	  insert_line = insert_line + 1

            nIdx = insert_line
            while(nIdx < ln)
            {
                szCurLine = GetBufLine(hbuf, nIdx);
                nRet = strstr(szCurLine, declare_word)
                if( nRet != 0xffffffff )
                {
                    //MSG("already declared")
                    nIdx = 0
                    break;
                }
                nIdx = nIdx + 1
            }
            if (nIdx != 0)
            {
            	  padding_cnt = GetPaddingSpace(GetBufLine(hbuf, symbol.lnName));
            	  InsBufLine(hbuf, insert_line + 1, TabSpace(padding_cnt + 1) # "@declare_word@")
            	  insert_decal = 1
            }
        }
        PutBufLine(hbuf, ln + insert_decal, GetBufLine(hbuf, ln + insert_decal) # "r ( @iname@ = 0; @iname@ < # ; @iname@++ )")
    }
    else if (szCmd == "switch" || szCmd == "sw")
    {
        nSwitch = ask("������case�ĸ���")
        SetBufSelText(hbuf, " ( # )")
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{")
        InsertMultiCaseProc(hbuf,szLine1,nSwitch)
    }
    else if (szCmd == "do")
    {
        InsBufLine(hbuf, ln + 1, "@szLine1@" # "{")
        InsBufLine(hbuf, ln + 2, "@szLine@" # "#");
        InsBufLine(hbuf, ln + 3, "@szLine1@" # "} while ( # );")
    }
    else if (szCmd == "case" || szCmd == "ca" )
    {
        SetBufSelText(hbuf, " # :")
        InsBufLine(hbuf, ln + 1, "@szLine@" # "#")
        InsBufLine(hbuf, ln + 2, "@szLine@" # "break;")
    }
    else if (szCmd == "struct" || szCmd == "st" || szCmd == "ste")
    {
        DelBufLine(hbuf, ln)
        //szStructName = toupper(Ask("������ṹ��:"))
        szStructName = Ask("������ṹ��:")
        //InsBufLine(hbuf, ln, "@szLine1@typedef struct @szStructName@");
        if (szCmd == "ste")
        	InsBufLine(hbuf, ln, "typedef @szLine1@struct yf_@szStructName@_s");
        else
        	InsBufLine(hbuf, ln, "@szLine1@struct yf_@szStructName@_s");
        InsBufLine(hbuf, ln + 1, "@szLine1@{");
        InsBufLine(hbuf, ln + 2, "@szLine1@" # TabSpace(1));
        //szStructName = cat(szStructName,"_STRU")
        //InsBufLine(hbuf, ln + 3, "@szLine1@}@szStructName@;");
        if (szCmd == "ste")
        {
        	InsBufLine(hbuf, ln + 3, "@szLine1@}");
        	InsBufLine(hbuf, ln + 4, "@szLine1@yf_@szStructName@_t;");
        }
        else
        	InsBufLine(hbuf, ln + 3, "@szLine1@};");
        SetBufIns (hbuf, ln + 2, strlen(szLine))
        return
    }
    else if (szCmd == "class" || szCmd == "cl" || szCmd == "clh")
    {
    	 DelBufLine(hbuf, ln)
    	 space_mb = "@szLine1@" # TabSpace(1)
    	 szClassName = Ask("����������:")
        InsBufLine(hbuf, ln, "@szLine1@class @szClassName@_cl");
        InsBufLine(hbuf, ln + 1, "@szLine1@{");
        InsBufLine(hbuf, ln + 2, "@szLine1@public:");

        line_index = ln + 3
        InsBufLine(hbuf, line_index, "@space_mb@@szClassName@_cl()");
        if (szCmd == "clh") {
        	PutBufLine(hbuf, line_index, GetBufLine(hbuf, line_index) # ";");
        }
        else {
        	InsBufLine(hbuf, line_index + 1, "@space_mb@{");
        	InsBufLine(hbuf, line_index + 2, "@space_mb@}");
        	line_index = line_index + 2
        }
        line_index = line_index + 1
        
        InsBufLine(hbuf, line_index, "@space_mb@~@szClassName@_cl()");
        if (szCmd == "clh") {
        	PutBufLine(hbuf, line_index, GetBufLine(hbuf, line_index) # ";");
        }
        else {
        	InsBufLine(hbuf, line_index + 1, "@space_mb@{");
        	InsBufLine(hbuf, line_index + 2, "@space_mb@}");
        	line_index = line_index + 2
        }
        line_index = line_index + 1

        InsBufLine(hbuf, line_index, "");
        InsBufLine(hbuf, line_index + 1, "@szLine1@private:");
        InsBufLine(hbuf, line_index + 2, "@szLine1@};");
        InsBufLine(hbuf, line_index + 3, "");
        SetBufIns(hbuf, line_index + 1, strlen(GetBufLine(hbuf, line_index + 1)))
        return
    }
    else if (szCmd == "enum" || szCmd == "en")
    {
        DelBufLine(hbuf, ln)
        //��ʾ����ö������ת��Ϊ��д
        szStructName = toupper(Ask("������ö����:"))
        InsBufLine(hbuf, ln, "@szLine1@enum @szStructName@_E");
        InsBufLine(hbuf, ln + 1, "@szLine1@{");
        InsBufLine(hbuf, ln + 2, "@szLine@" # TabSpace(1) #);
        //szStructName = cat(szStructName,"_ENUM")
        InsBufLine(hbuf, ln + 3, "@szLine1@};");
        SetBufIns (hbuf, ln + 2, strlen(szLine))
        return
    }
    else if (szCmd == "file" || szCmd == "fi" )
    {
        DelBufLine(hbuf, ln)
        /*�����ļ�ͷ˵��*/
        InsertFileHeaderCN( hbuf,0, szMyName,"" )
        return
    }
    else if (szCmd == "hd")
    {
        DelBufLine(hbuf, ln)
        InsFileHead(hbuf)    
        return
    }
    else if (szCmd == "hdn")
    {
        DelBufLine(hbuf, ln)
        /*���ɲ�Ҫ�ļ�������ͷ�ļ�*/
        CreateNewHeaderFile()
        return
    }
    else if (szCmd == "func" || szCmd == "fu")
    {
        DelBufLine(hbuf,ln)
        lnMax = GetBufLineCount(hbuf)
        if(ln != lnMax)
        {
            szNextLine = GetBufLine(hbuf,ln)
            /*����2.1���si����ǷǷ�symbol�ͻ��ж�ִ�У��ʸ�Ϊ�Ժ�һ��
              �Ƿ��С��������ж��Ƿ����º���*/
            if( (strstr(szNextLine,"(") != 0xffffffff) || (nVer != 2))
            {
                /*���Ѿ����ڵĺ���*/
                symbol = GetCurSymbol()
                if(strlen(symbol) != 0)
                {  
                    FuncHeadCommentCN(hbuf, ln, symbol, szMyName,0)
                    return
                }
            }
        }
        szFuncName = Ask("�����뺯������:")
        /*���º���*/
        FuncHeadCommentCN(hbuf, ln, szFuncName, szMyName, 1)
    }
    else if (szCmd == "tab") /*��tab��չΪ�ո�*/
    {
        DelBufLine(hbuf, ln)
        ReplaceBufTab()
    }
    else if (szCmd == "ap")
    {   
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day
        
        DelBufLine(hbuf, ln)
        szQuestion = AddPromblemNo()
        InsBufLine(hbuf, ln, "@szLine1@/* �� �� ��: @szQuestion@     �޸���:@szMyName@,   ʱ��:@sz@/@sz1@/@sz3@ ");
        szContent = Ask("�޸�ԭ��")
        szLeft = cat(szLine1,"   �޸�ԭ��: ");
        if(strlen(szLeft) > 70)
        {
            Msg("�ұ߿ռ�̫С,�����µ���")
            stop 
        }
        ln = CommentContent(hbuf,ln + 1,szLeft,szContent,1)
        return
    }
    else if (szCmd == "ab")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day
        
        DelBufLine(hbuf, ln)
        szQuestion = GetReg ("PNO")
        if(strlen(szQuestion)>0)
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Added by @szMyName@, @sz@/@sz1@/@sz3@   ���ⵥ��:@szQuestion@ */");
        }
        else
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Added by @szMyName@, @sz@/@sz1@/@sz3@ */");        
        }
        return
    }
    else if (szCmd == "ae")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day

        DelBufLine(hbuf, ln)
        InsBufLine(hbuf, ln, "@szLine1@/* END:   Added by @szMyName@, @sz@/@sz1@/@sz3@ */");
        return
    }
    else if (szCmd == "db")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day

        DelBufLine(hbuf, ln)
        szQuestion = GetReg ("PNO")
        if(strlen(szQuestion) > 0)
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Deleted by @szMyName@, @sz@/@sz1@/@sz3@   ���ⵥ��:@szQuestion@ */");
        }
        else
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Deleted by @szMyName@, @sz@/@sz1@/@sz3@ */");
        }
        
        return
    }
    else if (szCmd == "de")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day

        DelBufLine(hbuf, ln + 0)
        InsBufLine(hbuf, ln, "@szLine1@/* END: Deleted by @szMyName@, @sz@/@sz1@/@sz3@ */");
        return
    }
    else if (szCmd == "mb")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day

        DelBufLine(hbuf, ln)
        szQuestion = GetReg ("PNO")
        if(strlen(szQuestion) > 0)
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Modified by @szMyName@, @sz@/@sz1@/@sz3@   ���ⵥ��:@szQuestion@ */");
        }
        else
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Modified by @szMyName@, @sz@/@sz1@/@sz3@ */");
        }
        return
    }
    else if (szCmd == "me")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day
        //MSG(SysTime.hour)

        DelBufLine(hbuf, ln)
        InsBufLine(hbuf, ln, "@szLine1@/* END:   Modified by @szMyName@, @sz@/@sz1@/@sz3@ */");
        return
    }
    else
    {
    	 cur_symbol = GetCurSymbol()
    	 symbol_name = szCmd
    	 if (strlen(cur_symbol) > 0)
    	 	symbol_name = cur_symbol # "." # szCmd
    	 	
    	 symbol_ins = GetSymbolLocation(symbol_name)
    	 if (strlen(symbol_ins) > 0)
    	 {
    	 	if (symbol_ins.type == "Class")
    	 	{
    	 		DelBufLine(hbuf, ln)
    	 		declare_buf = OpenBuf(symbol_ins.file)
    	 		AutoCompleteClass(declare_buf, hbuf, ln, szCmd, symbol_ins)
    	 		CloseBuf(declare_buf)
    	 		return;
    	 	}
    	 }
        SearchForward()
        stop
    }
    SetWndSel(hwnd, sel)
    SearchForward()
}

macro ImpleteFunc(hbuf, htarget_buf, ln, cls_name, target_sym)
{
	last_buf = GetBufLine(hbuf, target_sym.lnLim-1)
	end_index = strstr(last_buf, ";")
	if (end_index == 0xffffffff) {
		return ln;
	}
	last_buf[end_index] = " "
	last_buf = TrimRight(last_buf)
	if (last_buf[strlen(last_buf) - 1] == "0") {
		//MSG("pure virtual method")
		return ln;
	}

	first_buf = last_buf
	if (target_sym.lnFirst + 1 < target_sym.lnLim)
		first_buf = GetBufLine(hbuf, target_sym.lnFirst)

	first_buf = TrimLeft(first_buf)
	if (strstr(first_buf, "virtual ") == 0)
	{
		first_buf = TrimLeft(strmid(first_buf, 8, strlen(first_buf)));
	}
	empty_flag = 0
	if (cls_name != "")
	{
		end_index = strstr(first_buf, "(")
		if (end_index == 0xffffffff) {
			return ln;
		}
		
		end_index = end_index - 1
		while (end_index >= 0) 
		{
			if (first_buf[end_index] == " " || first_buf[end_index] == "\t")
			{
				if (empty_flag == 1)
					break
			}
			else
				empty_flag = 1
			end_index = end_index - 1
		}
		if (end_index > 0)
		{
			end_index = end_index + 1
			first_buf = strmid(first_buf, 0, end_index) # "@cls_name@::" # strmid(first_buf, end_index, strlen(first_buf))
		}
		else {
			first_buf = "@cls_name@::" # first_buf
		}
	}

	first_buf = CommentDefaultParam(first_buf)
	InsBufLine(htarget_buf, ln, first_buf)
	ln = ln + 1

	if (target_sym.lnFirst + 1 < target_sym.lnLim)
	{
		ln_tmp = target_sym.lnFirst + 1
		while (ln_tmp + 1 < target_sym.lnLim)
		{
			InsBufLine(htarget_buf, ln, CommentDefaultParam(GetBufLine(hbuf, ln_tmp)))
			ln_tmp = ln_tmp + 1
			ln = ln + 1
		}
		InsBufLine(htarget_buf, ln, CommentDefaultParam(last_buf))
		ln = ln + 1
	}
	InsBufLine(htarget_buf, ln, "{")
	InsBufLine(htarget_buf, ln + 1, TabSpace(1))
	InsBufLine(htarget_buf, ln + 2, "}")
	InsBufLine(htarget_buf, ln + 3, "")
	return  ln + 4
}

macro CommentDefaultParam(buf)
{
	pre_buf = ""
	rest_buf = buf
	while (1)
	{
		start_index = strstr(rest_buf, "=")
		if (start_index == 0xffffffff)
			return  pre_buf # rest_buf;

		//MSG(start_index)
		buf_len = strlen(rest_buf)
		end_index = start_index + 1
		while (end_index < buf_len)
		{
			if (rest_buf[end_index] == "," || rest_buf[end_index] == ")")
				break;
			end_index = end_index + 1
		}
		pre_buf = pre_buf # strmid(rest_buf, 0, start_index) # "/*" # strmid(rest_buf, start_index, end_index) # "*/"
		rest_buf = strmid(rest_buf, end_index, buf_len)
	}
}

macro AutoCompleteClass(hbuf, htarget_buf, ln, cls_name, symbol_ins)
{
	//MSG(symbol_ins)
	
	hsyml = SymbolChildren(symbol_ins)
	cchild = SymListCount(hsyml)
	ichild = 0
	while (ichild < cchild)
	{
		childsym = SymListItem(hsyml, ichild)
		if (childsym.type == "Method Prototype")
		{
			ln = ImpleteFunc(hbuf, htarget_buf, ln, cls_name, childsym)
		}
		ichild = ichild + 1
	}
	SymListFree(hsyml)
}

macro InsIncludeHead(hbuf, ln)
{
	file_name = MyGetBufFile(hbuf)
	dot_index = strstr(file_name, ".")
	//MSG(file_name # ", " # dot_index # ", ln=" # ln) //note, "if + return" must with {}
	if (dot_index == 0xffffffff) {
		return;
	}

	file_name = strmid(file_name, 0, dot_index)
	include_str = "#include \"@file_name@.h\""
	//MSG(include_str)
	InsBufLine(hbuf, ln, include_str)
	InsBufLine(hbuf, ln + 1, "");
	InsBufLine(hbuf, ln + 2, "");
	InsBufLine(hbuf, ln + 3, "");
	SetBufIns(hbuf, ln + 2, 0);
}

macro InsFileHead(hbuf)
{
	InsFileHeadWithName(hbuf, MyGetBufFile(hbuf))
}

macro InsFileHeadWithName(hbuf, file_name)
{
	//MSG(file_name)
	i = 0
	total_len = strlen(file_name)
	while (i < total_len)
	{
		if (file_name[i] == ".")
			file_name[i] = "_";
		i = i + 1
	}
	now_time = GetSysTime(1)
	if (strlen(now_time.Month) < 2)
		now_time.Month = "0" # now_time.Month
	if (strlen(now_time.Day) < 2)
		now_time.Day = "0" # now_time.Day
		
	now_date = now_time.Year # now_time.Month # now_time.Day
	inc_mac = "_" # toupper(file_name) # "_"  # now_date  #  "_H"
	
	InsBufLine(hbuf, 0, "#ifndef @inc_mac@")
	InsBufLine(hbuf, 1, "#define @inc_mac@")
	InsBufLine(hbuf, 2, "/*")
	InsBufLine(hbuf, 3, "* \copyright\@: kevin_zhong, mail:qq2000zhong\@gmail.com")
	InsBufLine(hbuf, 4, "* time: @now_date@-" # now_time.time)
	InsBufLine(hbuf, 5, "*/")	
	InsBufLine(hbuf, 6, "")
	InsBufLine(hbuf, 7, "")
	InsBufLine(hbuf, 8, "")
	AppendBufLine(hbuf, "#endif")
	AppendBufLine(hbuf, "")

	SetBufIns(hbuf, 7, 0)
}

macro BlockCommandProc()
{
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    sel = GetWndSel(hwnd)
    hbuf = GetWndBuf(hwnd)
    if(sel.lnFirst > 0)
    {
        ln = sel.lnFirst - 1
    }
    else
    {
        stop
    }
    szLine = GetBufLine(hbuf,ln)
    szLine = TrimString(szLine)
    if(szLine == "while" || szLine == "wh")
    {
        InsertWhile()   /*����while*/
    }
    else if(szLine == "do")
    {
        InsertDo()   //����do while���
    }
    else if(szLine == "for")
    {
        InsertFor()  //����for���
    }
    else if(szLine == "if")
    {
        InsertIf()   //����if���
    }
    else if(szLine == "el" || szLine == "else")
    {
        InsertElse()  //����else���
        DelBufLine(hbuf,ln)
        stop
    }
    else if((szLine == "#ifd") || (szLine == "#ifdef"))
    {
        InsIfdef()        //����#ifdef
        DelBufLine(hbuf,ln)
        stop
    }
    else if((szLine == "#ifn") || (szLine == "#ifndef"))
    {
        InsIfndef()        //����#ifdef
        DelBufLine(hbuf,ln)
        stop
    }    
    else if (szLine == "abg")
    {
        InsertReviseAdd()
        DelBufLine(hbuf, ln)
        stop
    }
    else if (szLine == "dbg")
    {
        InsertReviseDel()
        DelBufLine(hbuf, ln)
        stop
    }
    else if (szLine == "mbg")
    {
        InsertReviseMod()
        DelBufLine(hbuf, ln)
        stop
    }
    else if(szLine == "#if")
    {
        InsertPredefIf()
        DelBufLine(hbuf,ln)
        stop
    }
    DelBufLine(hbuf,ln)
    SearchForward()
    stop
}

macro RestoreCommand(hbuf,szCmd)
{
    if(szCmd == "ca")
    {
        SetBufSelText(hbuf, "se")
        szCmd = "case"
    }
    else if(szCmd == "sw") 
    {
        SetBufSelText(hbuf, "itch")
        szCmd = "switch"
    }
    else if(szCmd == "el")
    {
        SetBufSelText(hbuf, "se")
        szCmd = "else"
    }
    else if(szCmd == "wh")
    {
        SetBufSelText(hbuf, "ile")
        szCmd = "while"
    }
    return szCmd
}

macro SearchForward()
{
    LoadSearchPattern("#", 1, 0, 1);
    Search_Forward
}

macro SearchBackward()
{
    LoadSearchPattern("#", 1, 0, 1);
    Search_Backward
}

macro InsertFuncName()
{
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    sel = GetWndSel(hwnd)
    hbuf = GetWndBuf(hwnd)
    symbolname = GetCurSymbol()
    SetBufSelText (hbuf, symbolname)
}
macro strstr(str1,str2)
{
    i = 0
    j = 0
    len1 = strlen(str1)
    len2 = strlen(str2)
    if((len1 == 0) || (len2 == 0))
    {
        return 0xffffffff
    }
    while( i < len1)
    {
        if(str1[i] == str2[j])
        {
            while(j < len2)
            {
                j = j + 1
                if(str1[i+j] != str2[j]) 
                {
                    break
                }
            }     
            if(j == len2)
            {
                return i
            }
            j = 0
        }
        i = i + 1      
    }  
    return 0xffffffff
}

macro InsertTraceInfo()
{
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    hbuf = GetWndBuf(hwnd)
    sel = GetWndSel(hwnd)
    symbol = GetSymbolLocationFromLn(hbuf, sel.lnFirst)
    InsertTraceInCurFunction(hbuf,symbol)
}

macro InsertTraceInCurFunction(hbuf,symbol)
{
    ln = GetBufLnCur (hbuf)
    symbolname = symbol.Symbol
    nLineEnd = symbol.lnLim
    nExitCount = 1;
    InsBufLine(hbuf, ln, "    DebugTrace(\"\\r\\n |@symbolname@() entry--- \");")
    ln = ln + 1
    fIsEnd = 1
    fIsNeedPrt = 1
    fIsSatementEnd = 1
    szLeftOld = ""
    while(ln < nLineEnd)
    {
        szLine = GetBufLine(hbuf, ln)
        iCurLineLen = strlen(szLine)
        
        /*�޳����е�ע�����*/
        RetVal = SkipCommentFromString(szLine,fIsEnd)
        szLine = RetVal.szContent
        fIsEnd = RetVal.fIsEnd
        //�����Ƿ���return���
/*        ret =strstr(szLine,"return")
        if(ret != 0xffffffff)
        {
            if( (szLine[ret+6] == " " ) || (szLine[ret+6] == "\t" )
                || (szLine[ret+6] == ";" ) || (szLine[ret+6] == "(" ))
            {
                szPre = strmid(szLine,0,ret)
            }
            SetBufIns(hbuf,ln,ret)
            Paren_Right
            sel = GetWndSel(hwnd)
            if( sel.lnLast != ln )
            {
                GetbufLine(hbuf,sel.lnLast)
                RetVal = SkipCommentFromString(szLine,1)
                szLine = RetVal.szContent
                fIsEnd = RetVal.fIsEnd
            }
        }*/
        //�����߿հ״�С
        nLeft = GetLeftBlank(szLine)
        if(nLeft == 0)
        {
            szLeft = "    "
        }
        else
        {
            szLeft = strmid(szLine,0,nLeft)
        }
        szLine = TrimString(szLine)
        iLen = strlen(szLine)
        if(iLen == 0)
        {
            ln = ln + 1
            continue
        }
        szRet = GetFirstWord(szLine)
//        if( (szRet == "if") || (szRet == "else")
        //�����Ƿ���return���
//        ret =strstr(szLine,"return")
        
        if( szRet == "return")
        {
            if( fIsSatementEnd == 0)
            {
                fIsNeedPrt = 1
                InsBufLine(hbuf,ln+1,"@szLeftOld@}")
                szEnd = cat(szLeft,"DebugTrace(\"\\r\\n |@symbolname@() exit---: @nExitCount@ \");")
                InsBufLine(hbuf, ln, szEnd )
                InsBufLine(hbuf,ln,"@szLeftOld@{")
                nExitCount = nExitCount + 1
                nLineEnd = nLineEnd + 3
                ln = ln + 3
            }
            else
            {
                fIsNeedPrt = 0
                szEnd = cat(szLeft,"DebugTrace(\"\\r\\n |@symbolname@() exit---: @nExitCount@ \");")
                InsBufLine(hbuf, ln, szEnd )
                nExitCount = nExitCount + 1
                nLineEnd = nLineEnd + 1
                ln = ln + 1
            }
        }
        else
        {
	        ret =strstr(szLine,"}")
	        if( ret != 0xffffffff )
	        {
	            fIsNeedPrt = 1
	        }
        }
        
        szLeftOld = szLeft
        ch = szLine[iLen-1] 
        if( ( ch  == ";" ) || ( ch  == "{" ) 
             || ( ch  == ":" )|| ( ch  == "}" ) || ( szLine[0] == "#" ))
        {
            fIsSatementEnd = 1
        }
        else
        {
            fIsSatementEnd = 0
        }
        ln = ln + 1
    }
    
    //ֻҪǰ���return����һ��}��˵�������Ľ�βû�з��أ���Ҫ�ټ�һ�����ڴ�ӡ
    if(fIsNeedPrt == 1)
    {
        InsBufLine(hbuf, ln,  "    DebugTrace(\"\\r\\n |@symbolname@() exit---: @nExitCount@ \");")        
        InsBufLine(hbuf, ln,  "")        
    }
}

macro GetFirstWord(szLine)
{
    szLine = TrimLeft(szLine)
    nIdx = 0
    iLen = strlen(szLine)
    while(nIdx < iLen)
    {
        if( (szLine[nIdx] == " ") || (szLine[nIdx] == "\t") 
          || (szLine[nIdx] == ";") || (szLine[nIdx] == "(")
          || (szLine[nIdx] == ".") || (szLine[nIdx] == "{")
          || (szLine[nIdx] == ",") || (szLine[nIdx] == ":") )
        {
            return strmid(szLine,0,nIdx)
        }
        nIdx = nIdx + 1
    }
    return ""
    
}

macro AutoInsertTraceInfoInBuf()
{
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    sel = GetWndSel(hwnd)
    hbuf = GetWndBuf(hwnd)

    isymMax = GetBufSymCount(hbuf)
    isym = 0
    while (isym < isymMax) 
    {
        symbol = GetBufSymLocation(hbuf, isym)
        isCodeBegin = 0
        fIsEnd = 1
        isBlandLine = 0
        if(strlen(symbol) > 0)
        {
            if(symbol.Type == "Class Placeholder")
	        {
		        hsyml = SymbolChildren(symbol)
				cchild = SymListCount(hsyml)
				ichild = 0
		    	while (ichild < cchild)
				{
                    symbol = GetBufSymLocation(hbuf, isym)
    		        hsyml = SymbolChildren(symbol)
					childsym = SymListItem(hsyml, ichild)
                    ln = childsym.lnName 
                    isCodeBegin = 0
                    fIsEnd = 1
                    isBlandLine = 0
                    while( ln < childsym.lnLim )
                    {   
                        szLine = GetBufLine (hbuf, ln)
                        
                        //ȥ��ע�͵ĸ���
                        RetVal = SkipCommentFromString(szLine,fIsEnd)
        		        szNew = RetVal.szContent
        		        fIsEnd = RetVal.fIsEnd
                        if(isCodeBegin == 1)
                        {
                            szNew = TrimLeft(szNew)
                            //����Ƿ��ǿ�ִ�д��뿪ʼ
                            iRet = CheckIsCodeBegin(szNew)
                            if(iRet == 1)
                            {
                                if( isBlandLine != 0 )
                                {
                                    ln = isBlandLine
                                }
                                InsBufLine(hbuf,ln,"")
                                childsym.lnLim = childsym.lnLim + 1
                                SetBufIns(hbuf, ln+1 , 0)
                                InsertTraceInCurFunction(hbuf,childsym)
                                break
                            }
                            if(strlen(szNew) == 0) 
                            {
                                if( isBlandLine == 0 ) 
                                {
                                    isBlandLine = ln;
                                }
                            }
                            else
                            {
                                isBlandLine = 0
                            }
                        }
        		        //���ҵ������Ŀ�ʼ
        		        if(isCodeBegin == 0)
        		        {
            		        iRet = strstr(szNew,"{")
                            if(iRet != 0xffffffff)
                            {
                                isCodeBegin = 1
                            }
                        }
                        ln = ln + 1
                    }
                    ichild = ichild + 1
				}
		        SymListFree(hsyml)
	        }
            else if( ( symbol.Type == "Function") ||  (symbol.Type == "Method") )
            {
                ln = symbol.lnName     
                while( ln < symbol.lnLim )
                {   
                    szLine = GetBufLine (hbuf, ln)
                    
                    //ȥ��ע�͵ĸ���
                    RetVal = SkipCommentFromString(szLine,fIsEnd)
    		        szNew = RetVal.szContent
    		        fIsEnd = RetVal.fIsEnd
                    if(isCodeBegin == 1)
                    {
                        szNew = TrimLeft(szNew)
                        //����Ƿ��ǿ�ִ�д��뿪ʼ
                        iRet = CheckIsCodeBegin(szNew)
                        if(iRet == 1)
                        {
                            if( isBlandLine != 0 )
                            {
                                ln = isBlandLine
                            }
                            SetBufIns(hbuf, ln , 0)
                            InsertTraceInCurFunction(hbuf,symbol)
                            InsBufLine(hbuf,ln,"")
                            break
                        }
                        if(strlen(szNew) == 0) 
                        {
                            if( isBlandLine == 0 ) 
                            {
                                isBlandLine = ln;
                            }
                        }
                        else
                        {
                            isBlandLine = 0
                        }
                    }
    		        //���ҵ������Ŀ�ʼ
    		        if(isCodeBegin == 0)
    		        {
        		        iRet = strstr(szNew,"{")
                        if(iRet != 0xffffffff)
                        {
                            isCodeBegin = 1
                        }
                    }
                    ln = ln + 1
                }
            }
        }
        isym = isym + 1
    }
    
}

macro CheckIsCodeBegin(szLine)
{
    iLen = strlen(szLine)
    if(iLen == 0)
    {
        return 0
    }
    nIdx = 0
    nWord = 0
    if( (szLine[nIdx] == "(") || (szLine[nIdx] == "-") 
           || (szLine[nIdx] == "*") || (szLine[nIdx] == "+"))
    {
        return 1
    }
    if( szLine[nIdx] == "#" )
    {
        return 0
    }
    while(nIdx < iLen)
    {
        if( (szLine[nIdx] == " ")||(szLine[nIdx] == "\t") 
             || (szLine[nIdx] == "(")||(szLine[nIdx] == "{")
             || (szLine[nIdx] == ";") )
        {
            if(nWord == 0)
            {
                if( (szLine[nIdx] == "(")||(szLine[nIdx] == "{")
                         || (szLine[nIdx] == ";")  )
                {
                    return 1
                }
                szFirstWord = StrMid(szLine,0,nIdx)
                if(szFirstWord == "return")
                {
                    return 1
                }
            }
            while(nIdx < iLen)
            {
                if( (szLine[nIdx] == " ")||(szLine[nIdx] == "\t") )
                {
                    nIdx = nIdx + 1
                }
                else
                {
                    break
                }
            }
            nWord = nWord + 1
            if(nIdx == iLen)
            {
                return 1
            }
        }
        if(nWord == 1)
        {
            asciiA = AsciiFromChar("A")
            asciiZ = AsciiFromChar("Z")
            ch = toupper(szLine[nIdx])
            asciiCh = AsciiFromChar(ch)
            if( ( szLine[nIdx] == "_" ) || ( szLine[nIdx] == "*" )
                 || ( ( asciiCh >= asciiA ) && ( asciiCh <= asciiZ ) ) )
            {
                return 0
            }
            else
            {
                return 1
            }
        }
        nIdx = nIdx + 1
    }
    return 1
}
macro AutoInsertTraceInfoInPrj()
{
    hprj = GetCurrentProj()
    ifileMax = GetProjFileCount (hprj)
    ifile = 0
    while (ifile < ifileMax)
    {
        filename = GetProjFileName (hprj, ifile)
        szExt = toupper(GetFileNameExt(filename))
        if( (szExt == "C") || (szExt == "CPP") )
        {
            hbuf = OpenBuf (filename)
            if(hbuf != 0)
            {
                SetCurrentBuf(hbuf)
                AutoInsertTraceInfoInBuf()
            }
        }
        //�Զ�������ļ����ɸ�����Ҫ��
/*        if( IsBufDirty (hbuf) )
        {
            SaveBuf (hbuf)
        }
        CloseBuf(hbuf)*/
        ifile = ifile + 1
    }
}

macro RemoveTraceInfo()
{
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    sel = GetWndSel(hwnd)
    hbuf = GetWndBuf(hwnd)
    if(hbuf == hNil)
       stop
    symbolname = GetCurSymbol()
    symbol = GetSymbolLocationFromLn(hbuf, sel.lnFirst)
//    symbol = GetSymbolLocation (symbolname)
    nLineEnd = symbol.lnLim
    szEntry = "DebugTrace(\"\\r\\n |@symbolname@() entry--- \");"
    szExit = "DebugTrace(\"\\r\\n |@symbolname@() exit---:" 
    ln = symbol.lnName
    fIsEntry = 0
    while(ln < nLineEnd)
    {
        szLine = GetBufLine(hbuf, ln)
        
        /*�޳����е�ע�����*/
        RetVal = TrimString(szLine)
        if(fIsEntry == 0)
        {
            ret = strstr(szLine,szEntry)
            if(ret != 0xffffffff)
            {
                DelBufLine(hbuf,ln)
                nLineEnd = nLineEnd - 1
                fIsEntry = 1
                ln = ln + 1
                continue
            }
        }
        ret = strstr(szLine,szExit)
        if(ret != 0xffffffff)
        {
            DelBufLine(hbuf,ln)
            nLineEnd = nLineEnd - 1
        }
        ln = ln + 1
    }
}

macro RemoveCurBufTraceInfo()
{
    hbuf = GetCurrentBuf()
    isymMax = GetBufSymCount(hbuf)
    isym = 0
    while (isym < isymMax) 
    {
        isLastLine = 0
        symbol = GetBufSymLocation(hbuf, isym)
        fIsEnd = 1
        if(strlen(symbol) > 0)
        {
            if(symbol.Type == "Class Placeholder")
	        {
		        hsyml = SymbolChildren(symbol)
				cchild = SymListCount(hsyml)
				ichild = 0
		    	while (ichild < cchild)
				{
    		        hsyml = SymbolChildren(symbol)
					childsym = SymListItem(hsyml, ichild)
                    SetBufIns(hbuf,childsym.lnName,0)
                    RemoveTraceInfo()
					ichild = ichild + 1
				}
		        SymListFree(hsyml)
	        }
            else if( ( symbol.Type == "Function") ||  (symbol.Type == "Method") )
            {
                SetBufIns(hbuf,symbol.lnName,0)
                RemoveTraceInfo()
            }
        }
        isym = isym + 1
    }
}

macro RemovePrjTraceInfo()
{
    hprj = GetCurrentProj()
    ifileMax = GetProjFileCount (hprj)
    ifile = 0
    while (ifile < ifileMax)
    {
        filename = GetProjFileName (hprj, ifile)
        hbuf = OpenBuf (filename)
        if(hbuf != 0)
        {
            SetCurrentBuf(hbuf)
            RemoveCurBufTraceInfo()
        }
        //�Զ�������ļ����ɸ�����Ҫ��
/*        if( IsBufDirty (hbuf) )
        {
            SaveBuf (hbuf)
        }
        CloseBuf(hbuf)*/
        ifile = ifile + 1
    }
}

macro InsertFileHeaderEN(hbuf, ln,szName,szContent)
{
    
    hnewbuf = newbuf("")
    if(hnewbuf == hNil)
    {
        stop
    }
    GetFunctionList(hbuf,hnewbuf)
    InsBufLine(hbuf, ln + 0,  "/******************************************************************************")
    InsBufLine(hbuf, ln + 1,  "")
    InsBufLine(hbuf, ln + 2,  "  Copyright (C), 2001-2011, DCN Co., Ltd.")
    InsBufLine(hbuf, ln + 3,  "")
    InsBufLine(hbuf, ln + 4,  " ******************************************************************************")
    sz = GetFileName(GetBufName (hbuf))
    InsBufLine(hbuf, ln + 5,  "  File Name     : @sz@")
    InsBufLine(hbuf, ln + 6,  "  Version       : Initial Draft")
    InsBufLine(hbuf, ln + 7,  "  Author        : @szName@")
    SysTime = GetSysTime(1)
    sz=SysTime.Year
    sz1=SysTime.month
    sz3=SysTime.day
    InsBufLine(hbuf, ln + 8,  "  Created       : @sz@/@sz1@/@sz3@")
    InsBufLine(hbuf, ln + 9,  "  Last Modified :")
    szTmp = "  Description   : "
    nlnDesc = ln
    iLen = strlen (szContent)
    InsBufLine(hbuf, ln + 10, "  Description   : @szContent@")
    InsBufLine(hbuf, ln + 11, "  Function List :")
    
    //���뺯���б�
    ln = InsertFileList(hbuf,hnewbuf,ln + 12) - 12
    closebuf(hnewbuf)
    InsBufLine(hbuf, ln + 12, "  History       :")
    InsBufLine(hbuf, ln + 13, "  1.Date        : @sz@/@sz1@/@sz3@")
    InsBufLine(hbuf, ln + 14, "    Author      : @szName@")
    InsBufLine(hbuf, ln + 15, "    Modification: Created file")
    InsBufLine(hbuf, ln + 16, "")
    InsBufLine(hbuf, ln + 17, "******************************************************************************/")
    InsBufLine(hbuf, ln + 18, "")
    InsBufLine(hbuf, ln + 19, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 20, " * external variables                           *")
    InsBufLine(hbuf, ln + 21, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 22, "")
    InsBufLine(hbuf, ln + 23, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 24, " * external routine prototypes                  *")
    InsBufLine(hbuf, ln + 25, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 26, "")
    InsBufLine(hbuf, ln + 27, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 28, " * internal routine prototypes                  *")
    InsBufLine(hbuf, ln + 29, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 30, "")
    InsBufLine(hbuf, ln + 31, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 32, " * project-wide global variables                *")
    InsBufLine(hbuf, ln + 33, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 34, "")
    InsBufLine(hbuf, ln + 35, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 36, " * module-wide global variables                 *")
    InsBufLine(hbuf, ln + 37, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 38, "")
    InsBufLine(hbuf, ln + 39, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 40, " * constants                                    *")
    InsBufLine(hbuf, ln + 41, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 42, "")
    InsBufLine(hbuf, ln + 43, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 44, " * macros                                       *")
    InsBufLine(hbuf, ln + 45, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 46, "")
    InsBufLine(hbuf, ln + 47, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 48, " * routines' implementations                    *")
    InsBufLine(hbuf, ln + 49, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 50, "")
    if(iLen != 0)
    {
        return
    }
    
    //���û�й���������������ʾ����
    szContent = Ask("Description")
    SetBufIns(hbuf,nlnDesc + 14,0)
    DelBufLine(hbuf,nlnDesc +10)
    
    //ע���������,�Զ�����
    CommentContent(hbuf,nlnDesc + 10,"  Description   : ",szContent,0)
}


macro InsertFileHeaderCN(hbuf, ln,szName,szContent)
{
    hnewbuf = newbuf("")
    if(hnewbuf == hNil)
    {
        stop
    }
    GetFunctionList(hbuf,hnewbuf)
    InsBufLine(hbuf, ln + 0,  "/******************************************************************************")
    InsBufLine(hbuf, ln + 1,  "")
    InsBufLine(hbuf, ln + 2,  "                  ��Ȩ���� (C), 2001-2011, ���������������޹�˾")
    InsBufLine(hbuf, ln + 3,  "")
    InsBufLine(hbuf, ln + 4,  " ******************************************************************************")
    sz = GetFileName(GetBufName (hbuf))
    InsBufLine(hbuf, ln + 5,  "  �� �� ��   : @sz@")
    InsBufLine(hbuf, ln + 6,  "  �� �� ��   : ����")
    InsBufLine(hbuf, ln + 7,  "  ��    ��   : @szName@")
    SysTime = GetSysTime(1)
    szTime = SysTime.Date
    InsBufLine(hbuf, ln + 8,  "  ��������   : @szTime@")
    InsBufLine(hbuf, ln + 9,  "  ����޸�   :")
    iLen = strlen (szContent)
    nlnDesc = ln
    szTmp = "  ��������   : "
    InsBufLine(hbuf, ln + 10, "  ��������   : @szContent@")
    InsBufLine(hbuf, ln + 11, "  �����б�   :")
    
    //���뺯���б�
    ln = InsertFileList(hbuf,hnewbuf,ln + 12) - 12
    closebuf(hnewbuf)
    InsBufLine(hbuf, ln + 12, "  �޸���ʷ   :")
    InsBufLine(hbuf, ln + 13, "  1.��    ��   : @szTime@")

    if( strlen(szMyName)>0 )
    {
       InsBufLine(hbuf, ln + 14, "    ��    ��   : @szName@")
    }
    else
    {
       InsBufLine(hbuf, ln + 14, "    ��    ��   : #")
    }
    InsBufLine(hbuf, ln + 15, "    �޸�����   : �����ļ�")    
    InsBufLine(hbuf, ln + 16, "")
    InsBufLine(hbuf, ln + 17, "******************************************************************************/")
    InsBufLine(hbuf, ln + 18, "")
    InsBufLine(hbuf, ln + 19, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 20, " * ����ͷ�ļ�                                   *")
    InsBufLine(hbuf, ln + 21, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 22, "")
    InsBufLine(hbuf, ln + 23, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 24, " * �ⲿ����˵��                                 *")
    InsBufLine(hbuf, ln + 25, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 26, "")
    InsBufLine(hbuf, ln + 27, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 28, " * �ⲿ����ԭ��˵��                             *")
    InsBufLine(hbuf, ln + 29, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 30, "")
    InsBufLine(hbuf, ln + 31, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 32, " * �ڲ�����ԭ��˵��                             *")
    InsBufLine(hbuf, ln + 33, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 34, "")
    InsBufLine(hbuf, ln + 35, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 36, " * ȫ�ֱ���                                     *")
    InsBufLine(hbuf, ln + 37, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 38, "")
    InsBufLine(hbuf, ln + 39, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 40, " * ģ�鼶����                                   *")
    InsBufLine(hbuf, ln + 41, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 42, "")
    InsBufLine(hbuf, ln + 43, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 44, " * ��������                                     *")
    InsBufLine(hbuf, ln + 45, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 46, "")
    InsBufLine(hbuf, ln + 47, "/*----------------------------------------------*")
    InsBufLine(hbuf, ln + 48, " * �궨��                                       *")
    InsBufLine(hbuf, ln + 49, " *----------------------------------------------*/")
    InsBufLine(hbuf, ln + 50, "")
    if(strlen(szContent) != 0)
    {
        return
    }
    
    //���û�����빦�������Ļ���ʾ����
    szContent = Ask("�������ļ���������������")
    SetBufIns(hbuf,nlnDesc + 14,0)
    DelBufLine(hbuf,nlnDesc +10)
    
    //�Զ�������ʾ��������
    CommentContent(hbuf,nlnDesc+10,"  ��������   : ",szContent,0)
}

macro GetFunctionList(hbuf,hnewbuf)
{
    isymMax = GetBufSymCount (hbuf)
    isym = 0
    //����ȡ��ȫ���ĵ�ǰbuf���ű��е�ȫ������
    while (isym < isymMax) 
    {
        symbol = GetBufSymLocation(hbuf, isym)
        if(symbol.Type == "Class Placeholder")
        {
	        hsyml = SymbolChildren(symbol)
			cchild = SymListCount(hsyml)
			ichild = 0
	    	while (ichild < cchild)
			{
				childsym = SymListItem(hsyml, ichild)
                AppendBufLine(hnewbuf,childsym.symbol)
				ichild = ichild + 1
			}
	        SymListFree(hsyml)
        }
        if(strlen(symbol) > 0)
        {
            if( (symbol.Type == "Method") || 
                (symbol.Type == "Function") || ("Editor Macro" == symbol.Type) )
            {
                //ȡ�������Ǻ����ͺ�ķ���
                symname = symbol.Symbol
                //�����Ų��뵽��buf����������Ϊ�˼���V2.1
                AppendBufLine(hnewbuf,symname)
               }
           }
        isym = isym + 1
    }
}
macro InsertFileList(hbuf,hnewbuf,ln)
{
    if(hnewbuf == hNil)
    {
        return ln
    }
    isymMax = GetBufLineCount (hnewbuf)
    isym = 0
    while (isym < isymMax) 
    {
        szLine = GetBufLine(hnewbuf, isym)
        InsBufLine(hbuf,ln,"              @szLine@")
        ln = ln + 1
        isym = isym + 1
    }
    return ln 
}


macro CommentContent1 (hbuf,ln,szPreStr,szContent,isEnd)
{
    //���������еĶ���ı��ϲ�
    szClip = MergeString()
    //ȥ������Ŀո�
    szTmp = TrimString(szContent)
    //������봰���е������Ǽ������е�����˵���Ǽ���������
    ret = strstr(szClip,szTmp)
    if(ret == 0)
    {
        szContent = szClip
    }
    szLeftBlank = szPreStr
    iLen = strlen(szPreStr)
    k = 0
    while(k < iLen)
    {
        szLeftBlank[k] = " ";
        k = k + 1;
    }
    iLen = strlen (szContent)
    szTmp = cat(szPreStr,"#");
    if( iLen == 0)
    {
        InsBufLine(hbuf, ln, "@szTmp@")
    }
    else
    {
        i = 0
        while  (iLen - i > 75 - k )
        {
            j = 0
            while(j < 75 - k)
            {
                iNum = szContent[i + j]
                //��������ı���ɶԴ���
                if( AsciiFromChar (iNum)  > 160 )
                {
                   j = j + 2
                }
                else
                {
                   j = j + 1
                }
                if( (j > 70 - k) && (szContent[i + j] == " ") )
                {
                    break
                }
            }
            if( (szContent[i + j] != " " ) )
            {
                n = 0;
                iNum = szContent[i + j + n]
                while( (iNum != " " ) && (AsciiFromChar (iNum)  < 160))
                {
                    n = n + 1
                    if((n >= 3) ||(i + j + n >= iLen))
                         break;
                    iNum = szContent[i + j + n]
                   }
                if(n < 3)
                {
                    j = j + n 
                    sz1 = strmid(szContent,i,i+j)
                    sz1 = cat(szPreStr,sz1)                
                }
                else
                {
                    sz1 = strmid(szContent,i,i+j)
                    sz1 = cat(szPreStr,sz1)
                    if(sz1[strlen(sz1)-1] != "-")
                    {
                        sz1 = cat(sz1,"-")                
                    }
                }
            }
            else
            {
                sz1 = strmid(szContent,i,i+j)
                sz1 = cat(szPreStr,sz1)
            }
            InsBufLine(hbuf, ln, "@sz1@")
            ln = ln + 1
            szPreStr = szLeftBlank
            i = i + j
            while(szContent[i] == " ")
            {
                i = i + 1
            }
        }
        sz1 = strmid(szContent,i,iLen)
        sz1 = cat(szPreStr,sz1)
        if(isEnd)
        {
            sz1 = cat(sz1,"*/")
        }
        InsBufLine(hbuf, ln, "@sz1@")
    }
    return ln
}



macro CommentContent (hbuf,ln,szPreStr,szContent,isEnd)
{
    szLeftBlank = szPreStr
    iLen = strlen(szPreStr)
    k = 0
    while(k < iLen)
    {
        szLeftBlank[k] = " ";
        k = k + 1;
    }

    hNewBuf = newbuf("clip")
    if(hNewBuf == hNil)
        return       
    SetCurrentBuf(hNewBuf)
    PasteBufLine (hNewBuf, 0)
    lnMax = GetBufLineCount( hNewBuf )
    szTmp = TrimString(szContent)

    //�ж������������0��ʱ������Щ�汾�������⣬Ҫ�ų���
    if(lnMax != 0)
    {
        szLine = GetBufLine(hNewBuf , 0)
	    ret = strstr(szLine,szTmp)
	    if(ret == 0)
	    {
	        /*������봰����������Ǽ������һ����˵���Ǽ���������ȡ�������е���
	          ��*/
	        szContent = TrimString(szLine)
	    }
	    else
	    {
	        lnMax = 1
	    }	    
    }
    else
    {
        lnMax = 1
    }    
    szRet = ""
    nIdx = 0
    while ( nIdx < lnMax) 
    {
        if(nIdx != 0)
        {
            szLine = GetBufLine(hNewBuf , nIdx)
            szContent = TrimLeft(szLine)
               szPreStr = szLeftBlank
        }
        iLen = strlen (szContent)
        szTmp = cat(szPreStr,"#");
        if( (iLen == 0) && (nIdx == (lnMax - 1))
        {
            InsBufLine(hbuf, ln, "@szTmp@")
        }
        else
        {
            i = 0
            //��ÿ��75���ַ�����
            while  (iLen - i > 75 - k )
            {
                j = 0
                while(j < 75 - k)
                {
                    iNum = szContent[i + j]
                    if( AsciiFromChar (iNum)  > 160 )
                    {
                       j = j + 2
                    }
                    else
                    {
                       j = j + 1
                    }
                    if( (j > 70 - k) && (szContent[i + j] == " ") )
                    {
                        break
                    }
                }
                if( (szContent[i + j] != " " ) )
                {
                    n = 0;
                    iNum = szContent[i + j + n]
                    //����������ַ�ֻ�ܳɶԴ���
                    while( (iNum != " " ) && (AsciiFromChar (iNum)  < 160))
                    {
                        n = n + 1
                        if((n >= 3) ||(i + j + n >= iLen))
                             break;
                        iNum = szContent[i + j + n]
                    }
                    if(n < 3)
                    {
                        //�ֶκ�ֻ��С��3�����ַ������¶���������ȥ
                        j = j + n 
                        sz1 = strmid(szContent,i,i+j)
                        sz1 = cat(szPreStr,sz1)                
                    }
                    else
                    {
                        //����3���ַ��ļ����ַ��ֶ�
                        sz1 = strmid(szContent,i,i+j)
                        sz1 = cat(szPreStr,sz1)
                        if(sz1[strlen(sz1)-1] != "-")
                        {
                            sz1 = cat(sz1,"-")                
                        }
                    }
                }
                else
                {
                    sz1 = strmid(szContent,i,i+j)
                    sz1 = cat(szPreStr,sz1)
                }
                InsBufLine(hbuf, ln, "@sz1@")
                ln = ln + 1
                szPreStr = szLeftBlank
                i = i + j
                while(szContent[i] == " ")
                {
                    i = i + 1
                }
            }
            sz1 = strmid(szContent,i,iLen)
            sz1 = cat(szPreStr,sz1)
            if((isEnd == 1) && (nIdx == (lnMax - 1))
            {
                sz1 = cat(sz1," */")
            }
            InsBufLine(hbuf, ln, "@sz1@")
        }
        ln = ln + 1
        nIdx = nIdx + 1
    }
    closebuf(hNewBuf)
    return ln - 1
}

macro FormatLine()
{
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    sel = GetWndSel(hwnd)
    if(sel.ichFirst > 70)
    {
        Msg("ѡ��̫������")
        stop 
    }
    hbuf = GetWndBuf(hwnd)
    // get line the selection (insertion point) is on
    szCurLine = GetBufLine(hbuf, sel.lnFirst);
    lineLen = strlen(szCurLine)
    szLeft = strmid(szCurLine,0,sel.ichFirst)
    szContent = strmid(szCurLine,sel.ichFirst,lineLen)
    DelBufLine(hbuf, sel.lnFirst)
    CommentContent(hbuf,sel.lnFirst,szLeft,szContent,0)            

}

macro CreateBlankString(nBlankCount)
{
    szBlank=""
    nIdx = 0
    while(nIdx < nBlankCount)
    {
        szBlank = cat(szBlank," ")
        nIdx = nIdx + 1
    }
    return szBlank
}

macro TrimLeft(szLine)
{
    nLen = strlen(szLine)
    if(nLen == 0)
    {
        return szLine
    }
    nIdx = 0
    while( nIdx < nLen )
    {
        if( ( szLine[nIdx] != " ") && (szLine[nIdx] != "\t") )
        {
            break
        }
        nIdx = nIdx + 1
    }
    return strmid(szLine,nIdx,nLen)
}

macro TrimRight(szLine)
{
    nLen = strlen(szLine)
    if(nLen == 0)
    {
        return szLine
    }
    nIdx = nLen
    while( nIdx > 0 )
    {
        nIdx = nIdx - 1
        if( ( szLine[nIdx] != " ") && (szLine[nIdx] != "\t") )
        {
            break
        }
    }
    return strmid(szLine,0,nIdx+1)
}
macro TrimString(szLine)
{
    szLine = TrimLeft(szLine)
    szLIne = TrimRight(szLine)
    return szLine
}

macro GetFunctionDef(hbuf,symbol)
{
    ln = symbol.lnName
    szFunc = ""
    if(strlen(symbol) == 0)
    {
       return szFunc
    }
    fIsEnd = 1
//    msg(symbol)
    while(ln < symbol.lnLim)
    {
        szLine = GetBufLine (hbuf, ln)
        //ȥ����ע�͵�������
        RetVal = SkipCommentFromString(szLine,fIsEnd)
		szLine = RetVal.szContent
		szLine = TrimString(szLine)
		fIsEnd = RetVal.fIsEnd
        //�����{��ʾ��������ͷ������
        ret = strstr(szLine,"{")        
        if(ret != 0xffffffff)
        {
            szLine = strmid(szLine,0,ret)
            szFunc = cat(szFunc,szLine)
            break
        }
        szFunc = cat(szFunc,szLine)        
        ln = ln + 1
    }
    return szFunc
}


macro GetWordFromString(hbuf,szLine,nBeg,nEnd,chBeg,chSeparator,chEnd)
{
    if((nEnd > strlen(szLine) || (nBeg > nEnd))
    {
        return 0
    }
    nMaxLen = 0
    nIdx = nBeg
    //�ȶ�λ����ʼ�ַ���Ǵ�
    while(nIdx < nEnd)
    {
        if(szLine[nIdx] == chBeg)
        {
            break
        }
        nIdx = nIdx + 1
    }
    nBegWord = nIdx + 1
    
    //���ڼ��chBeg��chEnd��������
    iCount = 0
    
    nEndWord = 0
    //�Էָ���Ϊ��ǽ�������
    while(nIdx < nEnd)
    {
        if(szLine[nIdx] == chSeparator)
        {
           szWord = strmid(szLine,nBegWord,nIdx)
           szWord = TrimString(szWord)
           nLen = strlen(szWord)
           if(nMaxLen < nLen)
           {
               nMaxLen = nLen
           }
           AppendBufLine(hbuf,szWord)
           nBegWord = nIdx + 1
        }
        if(szLine[nIdx] == chBeg)
        {
            iCount = iCount + 1
        }
        if(szLine[nIdx] == chEnd)
        {
            iCount = iCount - 1
            nEndWord = nIdx
            if( iCount == 0 )
            {
                break
            }
        }
        nIdx = nIdx + 1
    }
    if(nEndWord > nBegWord)
    {
        szWord = strmid(szLine,nBegWord,nEndWord)
        szWord = TrimString(szWord)
        nLen = strlen(szWord)
        if(nMaxLen < nLen)
        {
            nMaxLen = nLen
        }
        AppendBufLine(hbuf,szWord)
    }
    return nMaxLen
}

macro FuncHeadCommentCN(hbuf, ln, szFunc, szMyName,newFunc)
{
    iIns = 0
    if(newFunc != 1)
    {
        symbol = GetSymbolLocationFromLn(hbuf, ln)
        if(strlen(symbol) > 0)
        {
            hTmpBuf = NewBuf("Tempbuf")
            if(hTmpBuf == hNil)
            {
                stop
            }
            //���ļ�����ͷ�����һ�в�ȥ����ע��
            szLine = GetFunctionDef(hbuf,symbol)            
            iBegin = symbol.ichName 
            //ȡ������ֵ����
            szTemp = strmid(szLine,0,iBegin)
            szTemp = TrimString(szTemp)
            szRet =  GetFirstWord(szTemp)
            if(symbol.Type == "Method")
            {
                szTemp = strmid(szTemp,strlen(szRet),strlen(szTemp))
                szTemp = TrimString(szTemp)
                if(szTemp == "::")
                {
                    szRet = ""
                }
            }
            if(toupper (szRet) == "MACRO")
            {
                //���ں귵��ֵ���⴦��
                szRet = ""
            }
            //�Ӻ���ͷ�������������
            nMaxParamSize = GetWordFromString(hTmpBuf,szLine,iBegin,strlen(szLine),"(",",",")")
            lnMax = GetBufLineCount(hTmpBuf)
            ln = symbol.lnFirst
            SetBufIns (hbuf, ln, 0)
        }
    }
    else
    {
        lnMax = 0
        szLine = ""
        szRet = ""
    }
    InsBufLine(hbuf, ln, "/*****************************************************************************")
    if( strlen(szFunc)>0 )
    {
        InsBufLine(hbuf, ln+1, " �� �� ��  : @szFunc@")
    }
    else
    {
        InsBufLine(hbuf, ln+1, " �� �� ��  : #")
    }
    oldln = ln
    InsBufLine(hbuf, ln+2, " ��������  : ")
    szIns = " �������  : "
    if(newFunc != 1)
    {
        //�����Ѿ����ڵĺ������뺯������
        i = 0
        while ( i < lnMax) 
        {
            szTmp = GetBufLine(hTmpBuf, i)
            nLen = strlen(szTmp);
            szBlank = CreateBlankString(nMaxParamSize - nLen + 2)
            szTmp = cat(szTmp,szBlank)
            ln = ln + 1
            szTmp = cat(szIns,szTmp)
            //MSG(szTmp)
            InsBufLine(hbuf, ln+2, "@szTmp@")
            iIns = 1
            szIns = "             "
            i = i + 1
        }    
        closebuf(hTmpBuf)
    }
    if(iIns == 0)
    {       
            ln = ln + 1
            InsBufLine(hbuf, ln+2, " �������  : ��")
    }
    InsBufLine(hbuf, ln+3, " �������  : ��")
    InsBufLine(hbuf, ln+4, " �� �� ֵ  : @szRet@")
    InsBufLine(hbuf, ln+5, " ���ú���  : ")
    InsBufLine(hbuf, ln+6, " ��������  : ")
    InsbufLIne(hbuf, ln+7, " ");
    InsBufLine(hbuf, ln+8, " �޸���ʷ      :")
    SysTime = GetSysTime(1);
    szTime = SysTime.Date

    InsBufLine(hbuf, ln+9, "  1.��    ��   : @szTime@")

    if( strlen(szMyName)>0 )
    {
       InsBufLine(hbuf, ln+10, "    ��    ��   : @szMyName@")
    }
    else
    {
       InsBufLine(hbuf, ln+10, "    ��    ��   : #")
    }
    InsBufLine(hbuf, ln+11, "    �޸�����   : �����ɺ���")    
    InsBufLine(hbuf, ln+12, "")    
    InsBufLine(hbuf, ln+13, "*****************************************************************************/")
    if ((newFunc == 1) && (strlen(szFunc)>0))
    {
        InsBufLine(hbuf, ln+14, "yf_u32_t  @szFunc@( # )")
        InsBufLine(hbuf, ln+15, "{");
        InsBufLine(hbuf, ln+16, "    #");
        InsBufLine(hbuf, ln+17, "}");
        SearchForward()
    }        
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    sel = GetWndSel(hwnd)
    sel.ichFirst = 0
    sel.ichLim = sel.ichFirst
    sel.lnFirst = ln + 14
    sel.lnLast = ln + 14        
    szContent = Ask("�����뺯����������������")
    setWndSel(hwnd,sel)
    DelBufLine(hbuf,oldln + 2)

    //��ʾ����Ĺ�����������
    newln = CommentContent(hbuf,oldln+2," ��������  : ",szContent,0) - 2
    ln = ln + newln - oldln
    if ((newFunc == 1) && (strlen(szFunc)>0))
    {
        isFirstParam = 1
            
        //��ʾ�����º����ķ���ֵ
        szRet = Ask("�����뷵��ֵ����")
        if(strlen(szRet) > 0)
        {
            PutBufLine(hbuf, ln+4, " �� �� ֵ  : @szRet@")            
            PutBufLine(hbuf, ln+14, "@szRet@ @szFunc@(   )")
            SetbufIns(hbuf,ln+14,strlen(szRet)+strlen(szFunc) + 3
        }
        szFuncDef = ""
        sel.ichFirst = strlen(szFunc)+strlen(szRet) + 3
        sel.ichLim = sel.ichFirst + 1
        //ѭ���������
        while (1)
        {
            szParam = ask("�����뺯��������")
            szParam = TrimString(szParam)
            szTmp = cat(szIns,szParam)
            szParam = cat(szFuncDef,szParam)
            sel.lnFirst = ln + 14
            sel.lnLast = ln + 14
            setWndSel(hwnd,sel)
            sel.ichFirst = sel.ichFirst + strlen(szParam)
            sel.ichLim = sel.ichFirst
            oldsel = sel
            if(isFirstParam == 1)
            {
                PutBufLine(hbuf, ln+2, "@szTmp@")
                isFirstParam  = 0
            }
            else
            {
                ln = ln + 1
                InsBufLine(hbuf, ln+2, "@szTmp@")
                oldsel.lnFirst = ln + 14
                oldsel.lnLast = ln + 14        
            }
            SetBufSelText(hbuf,szParam)
            szIns = "             "
            szFuncDef = ", "
            oldsel.lnFirst = ln + 16
            oldsel.lnLast = ln + 16
            oldsel.ichFirst = 4
            oldsel.ichLim = 5
            setWndSel(hwnd,oldsel)
        }
    }
    return ln + 17
}

macro FuncHeadCommentEN(hbuf, ln, szFunc, szMyName,newFunc)
{
    iIns = 0
    if(newFunc != 1)
    {
        symbol = GetSymbolLocationFromLn(hbuf, ln)
        if(strlen(symbol) > 0)
        {
            hTmpBuf = NewBuf("Tempbuf")
                
            //���ļ�����ͷ�����һ�в�ȥ����ע��
            szLine = GetFunctionDef(hbuf,symbol)            
            iBegin = symbol.ichName
            
            //ȡ������ֵ����
            szTemp = strmid(szLine,0,iBegin)
            szTemp = TrimString(szTemp)
            szRet =  GetFirstWord(szTemp)
            if(symbol.Type == "Method")
            {
                szTemp = strmid(szTemp,strlen(szRet),strlen(szTemp))
                szTemp = TrimString(szTemp)
                if(szTemp == "::")
                {
                    szRet = ""
                }
            }
            if(toupper (szRet) == "MACRO")
            {
                //���ں귵��ֵ���⴦��
                szRet = ""
            }
            
            //�Ӻ���ͷ�������������
            nMaxParamSize = GetWordFromString(hTmpBuf,szLine,iBegin,strlen(szLine),"(",",",")")
            lnMax = GetBufLineCount(hTmpBuf)
            ln = symbol.lnFirst
            SetBufIns (hbuf, ln, 0)
        }
    }
    else
    {
        lnMax = 0
        szRet = ""
        szLine = ""
    }
    InsBufLine(hbuf, ln, "/*****************************************************************************")
    InsBufLine(hbuf, ln+1, " Prototype    : @szFunc@")
    InsBufLine(hbuf, ln+2, " Description  : ")
    oldln  = ln 
    szIns = " Input        : "
    if(newFunc != 1)
    {
        //�����Ѿ����ڵĺ���������������
        i = 0
        while ( i < lnMax) 
        {
            szTmp = GetBufLine(hTmpBuf, i)
            nLen = strlen(szTmp);
            
            //�����������Ŀո�ʵ���Ƕ������Ĳ�����˵��
            szBlank = CreateBlankString(nMaxParamSize - nLen + 2)
            szTmp = cat(szTmp,szBlank)
            ln = ln + 1
            szTmp = cat(szIns,szTmp)
            InsBufLine(hbuf, ln+2, "@szTmp@")
            iIns = 1
            szIns = "                "
            i = i + 1
        }    
        closebuf(hTmpBuf)
    }
    if(iIns == 0)
    {       
            ln = ln + 1
            InsBufLine(hbuf, ln+2, " Input        : None")
    }
    InsBufLine(hbuf, ln+3, " Output       : None")
    InsBufLine(hbuf, ln+4, " Return Value : @szRet@")
    InsBufLine(hbuf, ln+5, " Calls        : ")
    InsBufLine(hbuf, ln+6, " Called By    : ")
    InsbufLIne(hbuf, ln+7, " ");
    
    SysTime = GetSysTime(1);
    sz1=SysTime.Year
    sz2=SysTime.month
    sz3=SysTime.day

    InsBufLine(hbuf, ln + 8, "  History        :")
    InsBufLine(hbuf, ln + 9, "  1.Date         : @sz1@/@sz2@/@sz3@")
    InsBufLine(hbuf, ln + 10, "    Author       : @szMyName@")
    InsBufLine(hbuf, ln + 11, "    Modification : Created function")
    InsBufLine(hbuf, ln + 12, "")    
    InsBufLine(hbuf, ln + 13, "*****************************************************************************/")
    if ((newFunc == 1) && (strlen(szFunc)>0))
    {
        InsBufLine(hbuf, ln+14, "yf_u32_t  @szFunc@( # )")
        InsBufLine(hbuf, ln+15, "{");
        InsBufLine(hbuf, ln+16, "    #");
        InsBufLine(hbuf, ln+17, "}");
        SearchForward()
    }        
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    sel = GetWndSel(hwnd)
    sel.ichFirst = 0
    sel.ichLim = sel.ichFirst
    sel.lnFirst = ln + 14
    sel.lnLast = ln + 14        
    szContent = Ask("Description")
    DelBufLine(hbuf,oldln + 2)
    setWndSel(hwnd,sel)
    newln = CommentContent(hbuf,oldln + 2," Description  : ",szContent,0) - 2
    ln = ln + newln - oldln
    if ((newFunc == 1) && (strlen(szFunc)>0))
    {
        //��ʾ���뺯������ֵ��
        szRet = Ask("Please input return value type")
        if(strlen(szRet) > 0)
        {
            PutBufLine(hbuf, ln+4, " Return Value : @szRet@")            
            PutBufLine(hbuf, ln+14, "@szRet@ @szFunc@( # )")
            SetbufIns(hbuf,ln+14,strlen(szRet)+strlen(szFunc) + 3
        }
        szFuncDef = ""
        isFirstParam = 1
        sel.ichFirst = strlen(szFunc)+strlen(szRet) + 3
        sel.ichLim = sel.ichFirst + 1

        //ѭ�������º����Ĳ���
        while (1)
        {
            szParam = ask("Please input parameter")
            szParam = TrimString(szParam)
            szTmp = cat(szIns,szParam)
            szParam = cat(szFuncDef,szParam)
            sel.lnFirst = ln + 14
            sel.lnLast = ln + 14
            setWndSel(hwnd,sel)
            sel.ichFirst = sel.ichFirst + strlen(szParam)
            sel.ichLim = sel.ichFirst
            oldsel = sel
            if(isFirstParam == 1)
            {
                PutBufLine(hbuf, ln+2, "@szTmp@")
                isFirstParam  = 0
            }
            else
            {
                ln = ln + 1
                InsBufLine(hbuf, ln+2, "@szTmp@")
                oldsel.lnFirst = ln + 14
                oldsel.lnLast = ln + 14        
            }
            SetBufSelText(hbuf,szParam)
            szIns = "                "
            szFuncDef = ", "
            oldsel.lnFirst = ln + 16
            oldsel.lnLast = ln + 16
            oldsel.ichFirst = 4
            oldsel.ichLim = 5
            setWndSel(hwnd,oldsel)
        }
    }
    return ln + 17
}
macro InsertHistory(hbuf,ln,language)
{
    iHistoryCount = 1
    isLastLine = ln
    i = 0
    while(ln-i>0)
    {
        szCurLine = GetBufLine(hbuf, ln-i);
        iBeg1 = strstr(szCurLine,"��    ��  ")
        iBeg2 = strstr(szCurLine,"Date      ")
        if((iBeg1 != 0xffffffff) || (iBeg2 != 0xffffffff))
        {
            iHistoryCount = iHistoryCount + 1
            i = i + 1
            continue
        }
        iBeg1 = strstr(szCurLine,"�޸���ʷ")
        iBeg2 = strstr(szCurLine,"History      ")
        if((iBeg1 != 0xffffffff) || (iBeg2 != 0xffffffff))
        {
            break
        }
        iBeg = strstr(szCurLine,"/**********************")
        if( iBeg != 0xffffffff )
        {
            break
        }
        i = i + 1
    }
    if(language == 0)
    {
        InsertHistoryContentCN(hbuf,ln,iHistoryCount)
    }
    else
    {
        InsertHistoryContentEN(hbuf,ln,iHistoryCount)
    }
}
macro UpdateFunctionList()
{
    hnewbuf = newbuf("")
    if(hnewbuf == hNil)
    {
        stop
    }
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    sel = GetWndSel(hwnd)
    hbuf = GetWndBuf(hwnd)
    GetFunctionList(hbuf,hnewbuf)
    ln = sel.lnFirst
    iHistoryCount = 1
    isLastLine = ln
    iTotalLn = GetBufLineCount (hbuf) 
    while(ln < iTotalLn)
    {
        szCurLine = GetBufLine(hbuf, ln);
        iLen = strlen(szCurLine)
        j = 0;
        while(j < iLen)
        {
            if(szCurLine[j] != " ")
                break
            j = j + 1
        }
        
        //���ļ�ͷ˵����ǰ�д���10���ո��Ϊ�����б��¼
        if(j > 10)
        {
            DelBufLine(hbuf, ln)   
        }
        else
        {
            break
        }
        iTotalLn = GetBufLineCount (hbuf) 
    }

    //���뺯���б�
    InsertFileList( hbuf,hnewbuf,ln )
    closebuf(hnewbuf)
 }

macro  InsertHistoryContentCN(hbuf,ln,iHostoryCount)
{
    SysTime = GetSysTime(1);
    szTime = SysTime.Date
    szMyName = getreg(MYNAME)

    InsBufLine(hbuf, ln, "")
    InsBufLine(hbuf, ln + 1, "  @iHostoryCount@.��    ��   : @szTime@")

    if( strlen(szMyName) > 0 )
    {
       InsBufLine(hbuf, ln + 2, "    ��    ��   : @szMyName@")
    }
    else
    {
       InsBufLine(hbuf, ln + 2, "    ��    ��   : #")
    }
       szContent = Ask("�������޸ĵ�����")
       CommentContent(hbuf,ln + 3,"    �޸�����   : ",szContent,0)
}


macro  InsertHistoryContentEN(hbuf,ln,iHostoryCount)
{
    SysTime = GetSysTime(1);
    szTime = SysTime.Date
    sz1=SysTime.Year
    sz2=SysTime.month
    sz3=SysTime.day
    szMyName = getreg(MYNAME)
    InsBufLine(hbuf, ln, "")
    InsBufLine(hbuf, ln + 1, "  @iHostoryCount@.Date         : @sz1@/@sz2@/@sz3@")

    InsBufLine(hbuf, ln + 2, "    Author       : @szMyName@")
       szContent = Ask("Please input modification")
       CommentContent(hbuf,ln + 3,"    Modification : ",szContent,0)
}

macro CreateFunctionDef(hbuf, szName, language)
{
    ln = 0

    //��õ�ǰû�к�׺���ļ���
    szFileName = GetFileNameNoExt(GetBufName (hbuf))
    if(strlen(szFileName) == 0)
    {    
        sz = ask("������ͷ�ļ���")
        szFileName = GetFileNameNoExt(sz)
        szExt = GetFileNameExt(szFileName)        
        szPreH = toupper (szFileName)
        szPreH = cat("__",szPreH)
        szExt = toupper(szExt)
        szPreH = cat(szPreH,"_@szExt@__")
    }
    szPreH = toupper (szFileName)
    sz = cat(szFileName,".h")
    szPreH = cat("__",szPreH)
    szPreH = cat(szPreH,"_H__")
    hOutbuf = NewBuf(sz) // create output buffer
    if (hOutbuf == 0)
        stop
    //�������ű�ȡ�ú�����
    SetCurrentBuf(hOutbuf)
    isymMax = GetBufSymCount(hbuf)
    isym = 0
    while (isym < isymMax) 
    {
        isLastLine = 0
        symbol = GetBufSymLocation(hbuf, isym)
        fIsEnd = 1
        if(strlen(symbol) > 0)
        {
            if(symbol.Type == "Class Placeholder")
	        {
		        hsyml = SymbolChildren(symbol)
				cchild = SymListCount(hsyml)
				ichild = 0
				szClassName = symbol.Symbol
                InsBufLine(hOutbuf, ln, "}")
			    InsBufLine(hOutbuf, ln, "{")
			    InsBufLine(hOutbuf, ln, "class @szClassName@")
			    ln = ln + 2
		    	while (ichild < cchild)
				{
					childsym = SymListItem(hsyml, ichild)
					childsym.Symbol = szClassName
                    ln = CreateClassPrototype(hbuf,ln,childsym)
					ichild = ichild + 1
				}
		        SymListFree(hsyml)
                InsBufLine(hOutbuf, ln + 1, "")
		        ln = ln + 2
	        }
            else if( symbol.Type == "Function" )
            {
                ln = CreateFuncPrototype(hbuf,ln,"extern",symbol)
            }
            else if( symbol.Type == "Method" ) 
            {
                szLine = GetBufline(hbuf,symbol.lnName)
                szClassName = GetLeftWord(szLine,symbol.ichName)
                symbol.Symbol = szClassName
                ln = CreateClassPrototype(hbuf,ln,symbol)            
            }
            
        }
        isym = isym + 1
    }
    InsertCPP(hOutbuf,0)
    HeadIfdefStr(szPreH)
    szContent = GetFileName(GetBufName (hbuf))
    if(language == 0)
    {
        szContent = cat(szContent," ��ͷ�ļ�")
        //�����ļ�ͷ˵��
        InsertFileHeaderCN(hOutbuf,0,szName,szContent)
    }
    else
    {
        szContent = cat(szContent," header file")
        //�����ļ�ͷ˵��
        InsertFileHeaderEN(hOutbuf,0,szName,szContent)        
    }
}


macro GetLeftWord(szLine,ichRight)
{
    if(ich == 0)
    {
        return ""
    }
    ich = ichRight
    while(ich > 0)
    {
        if( (szLine[ich] == " ") || (szLine[ich] == "\t")
            || ( szLine[ich] == ":") || (szLine[ich] == "."))

        {
            ich = ich - 1
            ichRight = ich
        }
        else
        {
            break
        }
    }    
    while(ich > 0)
    {
        if(szLine[ich] == " ")
        {
            ich = ich + 1
            break
        }
        ich = ich - 1
    }
    return strmid(szLine,ich,ichRight)
}
macro CreateClassPrototype(hbuf,ln,symbol)
{
    isLastLine = 0
    fIsEnd = 1
    hOutbuf = GetCurrentBuf()
    szLine = GetBufLine (hbuf, symbol.lnName)
    sline = symbol.lnFirst     
    szClassName = symbol.Symbol
    ret = strstr(szLine,szClassName)
    if(ret == 0xffffffff)
    {
        return ln
    }
    szPre = strmid(szLine,0,ret)
    szLine = strmid(szLine,symbol.ichName,strlen(szLine))
    szLine = cat(szPre,szLine)
    //ȥ��ע�͵ĸ���
    RetVal = SkipCommentFromString(szLine,fIsEnd)
    fIsEnd = RetVal.fIsEnd
    szNew = RetVal.szContent
    szLine = cat("    ",szLine)
    szNew = cat("    ",szNew)
    while((isLastLine == 0) && (sline < symbol.lnLim))
    {   
        i = 0
        j = 0
        iLen = strlen(szNew)
        while(i < iLen)
        {
            if(szNew[i]=="(")
            {
               j = j + 1;
            }
            else if(szNew[i]==")")
            {
                j = j - 1;
                if(j <= 0)
                {
                    //��������ͷ����
                    isLastLine = 1  
                    //ȥ����������ַ�
        	        szLine = strmid(szLine,0,i+1);
                    szLine = cat(szLine,";")
                    break
                }
            }
            i = i + 1
        }
        InsBufLine(hOutbuf, ln, "@szLine@")
        ln = ln + 1
        sline = sline + 1
        if(isLastLine != 1)
        {              
            //��������ͷ��û�н�����ȡһ��
            szLine = GetBufLine (hbuf, sline)
            //ȥ��ע�͵ĸ���
            RetVal = SkipCommentFromString(szLine,fIsEnd)
	        szNew = RetVal.szContent
	        fIsEnd = RetVal.fIsEnd
        }                    
    }
    return ln
}

macro CreateFuncPrototype(hbuf,ln,szType,symbol)
{
    isLastLine = 0
    hOutbuf = GetCurrentBuf()
    szLine = GetBufLine (hbuf,symbol.lnName)
    //ȥ��ע�͵ĸ���
    RetVal = SkipCommentFromString(szLine,fIsEnd)
    szNew = RetVal.szContent
    fIsEnd = RetVal.fIsEnd
    szLine = cat("@szType@ ",szLine)
    szNew = cat("@szType@ ",szNew)
    sline = symbol.lnFirst     
    while((isLastLine == 0) && (sline < symbol.lnLim))
    {   
        i = 0
        j = 0
        iLen = strlen(szNew)
        while(i < iLen)
        {
            if(szNew[i]=="(")
            {
               j = j + 1;
            }
            else if(szNew[i]==")")
            {
                j = j - 1;
                if(j <= 0)
                {
                    //��������ͷ����
                    isLastLine = 1  
                    //ȥ����������ַ�
        	        szLine = strmid(szLine,0,i+1);
                    szLine = cat(szLine,";")
                    break
                }
            }
            i = i + 1
        }
        InsBufLine(hOutbuf, ln, "@szLine@")
        ln = ln + 1
        sline = sline + 1
        if(isLastLine != 1)
        {              
            //��������ͷ��û�н�����ȡһ��
            szLine = GetBufLine (hbuf, sline)
            szLine = cat("         ",szLine)
            //ȥ��ע�͵ĸ���
            RetVal = SkipCommentFromString(szLine,fIsEnd)
	        szNew = RetVal.szContent
	        fIsEnd = RetVal.fIsEnd
        }                    
    }
    return ln
}


macro CreateNewHeaderFile()
{
    hbuf = GetCurrentBuf()
    language = getreg(LANGUAGE)
    if(language != 1)
    {
        language = 0
    }
    szName = getreg(MYNAME)
    if(strlen( szName ) == 0)
    {
        szMyName = Ask("Enter your name:")
        setreg(MYNAME, szMyName)
    }
    isymMax = GetBufSymCount(hbuf)
    isym = 0
    ln = 0
    //��õ�ǰû�к�׺���ļ���
    sz = ask("Please input header file name")
    sz = "@sz@.h"
    /*szFileName = GetFileNameNoExt(sz)
    szExt = GetFileNameExt(sz)        
    szPreH = toupper (szFileName)
    szPreH = cat("__",szPreH)
    szExt = toupper(szExt)
    szPreH = cat(szPreH,"_@szExt@__")*/
    hOutbuf = NewBuf(sz) // create output buffer
    if (hOutbuf == 0)
        stop

    SetCurrentBuf(hOutbuf)
    InsertCPP(hOutbuf,0)
    //HeadIfdefStr(szPreH)
    //MSG(sz)
    InsFileHeadWithName(hOutbuf, sz)
    
    //szContent = GetFileName(GetBufName (hbuf))
    /*if(language == 0)
    {
        szContent = cat(szContent," ��ͷ�ļ�")

        //�����ļ�ͷ˵��
        InsertFileHeaderCN(hOutbuf,0,szName,szContent)
    }
    else
    {
        szContent = cat(szContent," header file")

        //�����ļ�ͷ˵��
        InsertFileHeaderEN(hOutbuf,0,szName,szContent)        
    }*/

    lnMax = GetBufLineCount(hOutbuf)
    if(lnMax > 9)
    {
        ln = lnMax - 9
    }
    else
    {
        return
    }
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    sel = GetWndSel(hwnd)
    sel.lnFirst = ln
    sel.ichFirst = 0
    sel.ichLim = 0
    SetBufIns(hOutbuf,ln,0)
    szType = Ask ("Please prototype type : extern or static")
    //�������ű�ȡ�ú�����
    while (isym < isymMax) 
    {
        isLastLine = 0
        symbol = GetBufSymLocation(hbuf, isym)
        fIsEnd = 1
        if(strlen(symbol) > 0)
        {
            if(symbol.Type == "Class Placeholder")
	        {
		        hsyml = SymbolChildren(symbol)
				cchild = SymListCount(hsyml)
				ichild = 0
				szClassName = symbol.Symbol
                InsBufLine(hOutbuf, ln, "}")
			    InsBufLine(hOutbuf, ln, "{")
			    InsBufLine(hOutbuf, ln, "class @szClassName@")
			    ln = ln + 2
		    	while (ichild < cchild)
				{
					childsym = SymListItem(hsyml, ichild)
					childsym.Symbol = szClassName
                    ln = CreateClassPrototype(hbuf,ln,childsym)
					ichild = ichild + 1
				}
		        SymListFree(hsyml)
                InsBufLine(hOutbuf, ln + 1, "")
		        ln = ln + 2
	        }
            else if( symbol.Type == "Function" )
            {
                ln = CreateFuncPrototype(hbuf,ln,szType,symbol)
            }
            else if( symbol.Type == "Method" ) 
            {
                szLine = GetBufline(hbuf,symbol.lnName)
                szClassName = GetLeftWord(szLine,symbol.ichName)
                symbol.Symbol = szClassName
                ln = CreateClassPrototype(hbuf,ln,symbol)            
            }
        }
        isym = isym + 1
    }
    sel.lnLast = ln 
    SetWndSel(hwnd,sel)
}


macro GetWordLeftOfIch(ich, sz)
{
    wordinfo = "" // create a "wordinfo" structure
    
    chTab = CharFromAscii(9)
    
    // scan backwords over white space, if any
    ich = ich - 1;
    if (ich >= 0)
        while (sz[ich] == " " || sz[ich] == chTab)
        {
            ich = ich - 1;
            if (ich < 0)
                break;
        }
    
    // scan backwords to start of word    
    ichLim = ich + 1;
    asciiA = AsciiFromChar("A")
    asciiZ = AsciiFromChar("Z")
    while (ich >= 0)
    {
        ch = toupper(sz[ich])
        asciiCh = AsciiFromChar(ch)
        
/*        if ((asciiCh < asciiA || asciiCh > asciiZ)
             && !IsNumber(ch)
             &&  (ch != "#") )
            break // stop at first non-identifier character
*/
        //ֻ��ȡ�ַ���# { / *��Ϊ����
        if ((asciiCh < asciiA || asciiCh > asciiZ) 
           && !IsNumber(ch)
           && ( ch != "#" && ch != "{" && ch != "/" && ch != "*"))
            break;

        ich = ich - 1;
    }
    
    ich = ich + 1
    wordinfo.szWord = strmid(sz, ich, ichLim)
    wordinfo.ich = ich
    wordinfo.ichLim = ichLim;
    
    return wordinfo
}


macro ReplaceBufTab()
{
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    hbuf = GetWndBuf(hwnd)
    iTotalLn = GetBufLineCount (hbuf)
    nBlank = Ask("һ��Tab�滻�����ո�")
    if(nBlank == 0)
    {
        nBlank = 4
    }
    szBlank = CreateBlankString(nBlank)
    ReplaceInBuf(hbuf,"\t",szBlank,0, iTotalLn, 1, 0, 0, 1)
}

macro ReplaceTabInProj()
{
    hprj = GetCurrentProj()
    ifileMax = GetProjFileCount (hprj)
    nBlank = Ask("һ��Tab�滻�����ո�")
    if(nBlank == 0)
    {
        nBlank = 4
    }
    szBlank = CreateBlankString(nBlank)

    ifile = 0
    while (ifile < ifileMax)
    {
        filename = GetProjFileName (hprj, ifile)
        hbuf = OpenBuf (filename)
        if(hbuf != 0)
        {
            iTotalLn = GetBufLineCount (hbuf)
            ReplaceInBuf(hbuf,"\t",szBlank,0, iTotalLn, 1, 0, 0, 1)
        }
        if( IsBufDirty (hbuf) )
        {
            SaveBuf (hbuf)
        }
        CloseBuf(hbuf)
        ifile = ifile + 1
    }
}


macro ReplaceInBuf(hbuf,chOld,chNew,nBeg,nEnd,fMatchCase, fRegExp, fWholeWordsOnly, fConfirm)
{
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    hbuf = GetWndBuf(hwnd)
    sel = GetWndSel(hwnd)
    sel.ichLim = 0
    sel.lnLast = 0
    sel.ichFirst = sel.ichLim
    sel.lnFirst = sel.lnLast
    SetWndSel(hwnd, sel)
    LoadSearchPattern(chOld, 0, 0, 0);
    while(1)
    {
        Search_Forward
        selNew = GetWndSel(hwnd)
        if(sel == selNew)
        {
            break
        }
        SetBufSelText(hbuf, chNew)
           selNew.ichLim = selNew.ichFirst 
        SetWndSel(hwnd, selNew)
        sel = selNew
    }
}


macro ConfigureSystem()
{
    szLanguage = ASK("Please select language: 0 Chinese ,1 English");
    if(szLanguage == "#")
    {
       SetReg ("LANGUAGE", "0")
    }
    else
    {
       SetReg ("LANGUAGE", szLanguage)
    }
    
    szName = ASK("Please input your name");
    if(szName == "#")
    {
       SetReg ("MYNAME", "")
    }
    else
    {
       SetReg ("MYNAME", szName)
    }
}

macro GetLeftBlank(szLine)
{
    nIdx = 0
    nEndIdx = strlen(szLine)
    while( nIdx < nEndIdx )
    {
        if( (szLine[nIdx] !=" ") && (szLine[nIdx] !="\t") )
        {
            break;
        }
        nIdx = nIdx + 1
    }
    return nIdx
}

macro ExpandBraceLittle()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    if( (sel.lnFirst == sel.lnLast) 
        && (sel.ichFirst == sel.ichLim) )
    {
        SetBufSelText (hbuf, "(  )")
        SetBufIns (hbuf, sel.lnFirst, sel.ichFirst + 2)    
    }
    else
    {
        SetBufIns (hbuf, sel.lnFirst, sel.ichFirst)    
        SetBufSelText (hbuf, "( ")
        SetBufIns (hbuf, sel.lnLast, sel.ichLim + 2)    
        SetBufSelText (hbuf, " )")
    }
    
}

macro ExpandBraceMid()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    if( (sel.lnFirst == sel.lnLast) 
        && (sel.ichFirst == sel.ichLim) )
    {
        SetBufSelText (hbuf, "[]")
        SetBufIns (hbuf, sel.lnFirst, sel.ichFirst + 1)    
    }
    else
    {
        SetBufIns (hbuf, sel.lnFirst, sel.ichFirst)    
        SetBufSelText (hbuf, "[")
        SetBufIns (hbuf, sel.lnLast, sel.ichLim + 1)    
        SetBufSelText (hbuf, "]")
    }
    
}

macro ExpandBraceLarge()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    ln = sel.lnFirst
    nlineCount = 0
    retVal = ""
    szLine = GetBufLine( hbuf, ln )    
    nLeft = GetLeftBlank(szLine)
    szLeft = strmid(szLine,0,nLeft);
    szRight = ""
    szMid = ""
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        //����û�п�ѡ��������ֱ�Ӳ���{}����
        if( nLeft == strlen(szLine) )
        {
            SetBufSelText (hbuf, "{")
        }
        else
        {    
            ln = ln + 1        
            InsBufLine(hbuf, ln, "@szLeft@{")     
            nlineCount = nlineCount + 1

        }
        InsBufLine(hbuf, ln + 1, "@szLeft@    ")
        InsBufLine(hbuf, ln + 2, "@szLeft@}")
        nlineCount = nlineCount + 2
        SetBufIns (hbuf, ln + 1, strlen(szLeft)+4)
    }
    else
    {
        //�����п�ѡ���������ÿ��ǽ���ѡ�����ֿ���
        
        //���ѡ�������Ƿ��������ԣ������̫����ע�͵�������ж�
        RetVal= CheckBlockBrace(hbuf)
        if(RetVal.iCount != 0)
        {
            msg("Invalidated brace number")
            stop
        }
        
        //ȡ��ѡ����ǰ������
        szOld = strmid(szLine,0,sel.ichFirst)
        if(sel.lnFirst != sel.lnLast)
        {
            //���ڶ��е����
            
            //��һ�е�ѡ�в���
            szMid = strmid(szLine,sel.ichFirst,strlen(szLine))
            szMid = TrimString(szMid)
            szLast = GetBufLine(hbuf,sel.lnLast)
            if( sel.ichLim > strlen(szLast) )
            {
                //���ѡ�������ȴ��ڸ��еĳ��ȣ����ȡ���еĳ���
                szLineselichLim = strlen(szLast)
            }
            else
            {
                szLineselichLim = sel.ichLim
            }
            
            //�õ����һ��ѡ����Ϊ���ַ�
            szRight = strmid(szLast,szLineselichLim,strlen(szLast))
            szRight = TrimString(szRight)
        }
        else
        {
            //����ѡ��ֻ��һ�е����
             if(sel.ichLim >= strlen(szLine))
             {
                 sel.ichLim = strlen(szLine)
             }
             
             //���ѡ����������
             szMid = strmid(szLine,sel.ichFirst,sel.ichLim)
             szMid = TrimString(szMid)            
             if( sel.ichLim > strlen(szLine) )
             {
                 szLineselichLim = strlen(szLine)
             }
             else
             {
                 szLineselichLim = sel.ichLim
             }
             
             //ͬ���õ�ѡ�����������
             szRight = strmid(szLine,szLineselichLim,strlen(szLine))
             szRight = TrimString(szRight)
        }
        nIdx = sel.lnFirst
        while( nIdx < sel.lnLast)
        {
            szCurLine = GetBufLine(hbuf,nIdx+1)
            if( sel.ichLim > strlen(szCurLine) )
            {
                szLineselichLim = strlen(szCurLine)
            }
            else
            {
                szLineselichLim = sel.ichLim
            }
            szCurLine = cat("    ",szCurLine)
            if(nIdx == sel.lnLast - 1)
            {
                //�������һ��Ӧ����ѡ�����ڵ����ݺ�����λ
                szCurLine = strmid(szCurLine,0,szLineselichLim + 4)
                PutBufLine(hbuf,nIdx+1,szCurLine)                    
            }
            else
            {
                //������������е����ݺ�����λ
                PutBufLine(hbuf,nIdx+1,szCurLine)
            }
            nIdx = nIdx + 1
        }
        if(strlen(szRight) != 0)
        {
            //���������һ��û�б�ѡ�������
            InsBufLine(hbuf, sel.lnLast + 1, "@szLeft@@szRight@")        
        }
        InsBufLine(hbuf, sel.lnLast + 1, "@szLeft@}")        
        nlineCount = nlineCount + 1
        if(nLeft < sel.ichFirst)
        {
            //���ѡ����ǰ�����ݲ��ǿո���Ҫ�����ò�������
            PutBufLine(hbuf,ln,szOld)
            InsBufLine(hbuf, ln+1, "@szLeft@{")
            nlineCount = nlineCount + 1
            ln = ln + 1
        }
        else
        {
            //���ѡ����ǰû������ֱ��ɾ������
            DelBufLine(hbuf,ln)
            InsBufLine(hbuf, ln, "@szLeft@{")
        }
        if(strlen(szMid) > 0)
        {
            //�����һ��ѡ����������
            InsBufLine(hbuf, ln+1, "@szLeft@    @szMid@")
            nlineCount = nlineCount + 1
            ln = ln + 1
        }        
    }
    retVal.szLeft = szLeft
    retVal.nLineCount = nlineCount
    //������������ߵĿհ�
    return retVal
}

/*
macro ScanStatement(szLine,iBeg)
{
    nIdx = 0
    iLen = strlen(szLine)
    while(nIdx < iLen -1)
    {
        if(szLine[nIdx] == "/" && szLine[nIdx + 1] == "/")
        {
            return 0xffffffff
        }
        if(szLine[nIdx] == "/" && szLine[nIdx + 1] == "*")
        {
           while(nIdx < iLen)
           {
               if(szLine[nIdx] == "*" && szLine[nIdx + 1] == "/")
               {
                   break
               }
               nIdx = nIdx + 1
               
           }
        }
        if( (szLine[nIdx] != " ") && (szLine[nIdx] != "\t" ))
        {
            return nIdx
        }
        nIdx = nIdx + 1
    }
    if( (szLine[iLen -1] == " ") || (szLine[iLen -1] == "\t" ))
    {
        return 0xffffffff
    }
    return nIdx
}
*/
/*
macro MoveCommentLeftBlank(szLine)
{
    nIdx  = 0
    iLen = strlen(szLine)
    while(nIdx < iLen - 1)
    { 
        if(szLine[nIdx] == "/" && szLine[nIdx+1] == "*")
        {
            szLine[nIdx] = " "
            szLine[nIdx + 1] = " "
            nIdx = nIdx + 2
            while(nIdx < iLen - 1)
            {
                if(szLine[nIdx] != " " && szLine[nIdx] != "\t")
                {
                    szLine[nIdx - 2] = "/"
                    szLine[nIdx - 1] = "*"
                    return szLine
                }
                nIdx = nIdx + 1
            }
        
        }
        if(szLine[nIdx] == "/" && szLine[nIdx+1] == "/")
        {
            szLine[nIdx] = " "
            szLine[nIdx + 1] = " "
            nIdx = nIdx + 2
            while(nIdx < iLen - 1)
            {
                if(szLine[nIdx] != " " && szLine[nIdx] != "\t")
                {
                    szLine[nIdx - 2] = "/"
                    szLine[nIdx - 1] = "/"
                    return szLine
                }
                nIdx = nIdx + 1
            }
        
        }
        nIdx = nIdx + 1
    }
    return szLine
}*/

macro DelCompoundStatement()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    ln = sel.lnFirst
    szLine = GetBufLine(hbuf,ln )
    nLeft = GetLeftBlank(szLine)
    szLeft = strmid(szLine,0,nLeft);
    Msg("@szLine@  will be deleted !")
    fIsEnd = 1
    while(1)
    {
        RetVal = SkipCommentFromString(szLine,fIsEnd)
        szTmp = RetVal.szContent
        fIsEnd = RetVal.fIsEnd
        //���Ҹ������Ŀ�ʼ
        ret = strstr(szTmp,"{")
        if(ret != 0xffffffff)
        {
            szNewLine = strmid(szLine,ret+1,strlen(szLine))
            szNew = strmid(szTmp,ret+1,strlen(szTmp))
            szNew = TrimString(szNew)
            if(szNew != "")
            {
                InsBufLine(hbuf,ln + 1,"@szLeft@    @szNewLine@");
            }
            sel.lnFirst = ln
            sel.lnLast = ln
            sel.ichFirst = ret
            sel.ichLim = ret
            //���Ҷ�Ӧ�Ĵ�����
            
            //ʹ���Լ���д�Ĵ����ٶ�̫��
            retTmp = SearchCompoundEnd(hbuf,ln,ret)
            if(retTmp.iCount == 0)
            {
                
                DelBufLine(hbuf,retTmp.ln)
                sel.ichFirst = 0
                sel.ichLim = 0
                DelBufLine(hbuf,ln)
                sel.lnLast = retTmp.ln - 1
                SetWndSel(hwnd,sel)
                Indent_Left
            }
            
            //ʹ��Si�Ĵ�������Է�������V2.1ʱ��ע��Ƕ��ʱ��������
/*            SetWndSel(hwnd,sel)
            Block_Down
            selNew = GetWndSel(hwnd)
            if(selNew != sel)
            {
                
                DelBufLine(hbuf,selNew.lnFirst)
                sel.ichFirst = 0
                sel.ichLim = 0
                DelBufLine(hbuf,ln)
                sel.lnLast = selNew.lnFirst - 1
                SetWndSel(hwnd,sel)
                Indent_Left
            }*/
            break
        }
        szTmp = TrimString(szTmp)
        iLen = strlen(szTmp)
        if(iLen != 0)
        {
            if(szTmp[iLen-1] == ";")
            {
                break
            }
        }
        DelBufLine(hbuf,ln)   
        if( ln == GetBufLineCount(hbuf ))
        {
             break
        }
        szLine = GetBufLine(hbuf,ln)
    }
}

macro CheckBlockBrace(hbuf)
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    ln = sel.lnFirst
    nCount = 0
    RetVal = ""
    szLine = GetBufLine( hbuf, ln )    
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        RetVal.iCount = 0
        RetVal.ich = sel.ichFirst
        return RetVal
    }
    if(sel.lnFirst == sel.lnLast && sel.ichFirst != sel.ichLim)
    {
        RetTmp = SkipCommentFromString(szLine,fIsEnd)
        szTmp = RetTmp.szContent
        RetVal = CheckBrace(szTmp,sel.ichFirst,sel.ichLim,"{","}",0,1)
        return RetVal
    }
    if(sel.lnFirst != sel.lnLast)
    {
	    fIsEnd = 1
	    while(ln <= sel.lnLast)
	    {
	        if(ln == sel.lnFirst)
	        {
	            RetVal = CheckBrace(szLine,sel.ichFirst,strlen(szLine)-1,"{","}",nCount,fIsEnd)
	        }
	        else if(ln == sel.lnLast)
	        {
	            RetVal = CheckBrace(szLine,0,sel.ichLim,"{","}",nCount,fIsEnd)
	        }
	        else
	        {
	            RetVal = CheckBrace(szLine,0,strlen(szLine)-1,"{","}",nCount,fIsEnd)
	        }
	        fIsEnd = RetVal.fIsEnd
	        ln = ln + 1
	        nCount = RetVal.iCount
	        szLine = GetBufLine( hbuf, ln )    
	    }
    }
    return RetVal
}

macro SearchCompoundEnd(hbuf,ln,ichBeg)
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    ln = sel.lnFirst
    nCount = 0
    SearchVal = ""
//    szLine = GetBufLine( hbuf, ln )
    lnMax = GetBufLineCount(hbuf)
    fIsEnd = 1
    while(ln < lnMax)
    {
        szLine = GetBufLine( hbuf, ln )
        RetVal = CheckBrace(szLine,ichBeg,strlen(szLine)-1,"{","}",nCount,fIsEnd)
        fIsEnd = RetVal.fIsEnd
        ichBeg = 0
        nCount = RetVal.iCount
        
        //���nCount=0��˵��{}����Ե�
        if(nCount == 0)
        {
            break
        }
        ln = ln + 1
//        szLine = GetBufLine( hbuf, ln )    
    }
    SearchVal.iCount = RetVal.iCount
    SearchVal.ich = RetVal.ich
    SearchVal.ln = ln
    return SearchVal
}

macro CheckBrace(szLine,ichBeg,ichEnd,chBeg,chEnd,nCheckCount,isCommentEnd)
{
    retVal = ""
    retVal.ich = 0
    nIdx = ichBeg
    nLen = strlen(szLine)
    if(ichEnd >= nLen)
    {
        ichEnd = nLen - 1
    }
    fIsEnd = 1
    while(nIdx <= ichEnd)
    {
        //�����/*ע�����������ö�
        if( (isCommentEnd == 0) || (szLine[nIdx] == "/" && szLine[nIdx+1] == "*"))
        {
            fIsEnd = 0
            while(nIdx <= ichEnd )
            {
                if(szLine[nIdx] == "*" && szLine[nIdx+1] == "/")
                {
                    nIdx = nIdx + 1 
                    fIsEnd  = 1
                    isCommentEnd = 1
                    break
                }
                nIdx = nIdx + 1 
            }
            if(nIdx > ichEnd)
            {
                break
            }
        }
        //�����//ע����ֹͣ����
        if(szLine[nIdx] == "/" && szLine[nIdx+1] == "/")
        {
            break
        }
        if(szLine[nIdx] == chBeg)
        {
            nCheckCount = nCheckCount + 1
        }
        if(szLine[nIdx] == chEnd)
        {
            nCheckCount = nCheckCount - 1
            if(nCheckCount == 0)
            {
                retVal.ich = nIdx
            }
        }
        nIdx = nIdx + 1
    }
    retVal.iCount = nCheckCount
    retVal.fIsEnd = fIsEnd
    return retVal
}

macro InsertElse()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    ln = sel.lnFirst
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        szLeft = CreateBlankString(sel.ichFirst)
        InsBufLine(hbuf, ln,szLeft)
        SetWndSel(hwnd,sel)
    }
    val = ExpandBraceLarge()
    szLeft = val.szLeft
    InsBufLine(hbuf, ln, "@szLeft@else")    
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        PutBufLine(hbuf,ln+2, "@szLeft@    ")
        SetBufIns (hbuf, ln+2, strlen(szLeft)+4)
        return
    }
    SetBufIns (hbuf, ln, strlen(szLeft)+7)
}

macro InsertCase()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    ln = sel.lnFirst
    szLine = GetBufLine( hbuf, ln )    
    nLeft = GetLeftBlank(szLine)
    szLeft = strmid(szLine,0,nLeft);
    InsBufLine(hbuf, ln, "@szLeft@" # "case # :")
    InsBufLine(hbuf, ln + 1, "@szLeft@" # "    " # "#")
    InsBufLine(hbuf, ln + 2, "@szLeft@" # "    " # "break;")
    SearchForward()    
}

macro InsertSwitch()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    ln = sel.lnFirst
    szLine = GetBufLine( hbuf, ln )    
    nLeft = GetLeftBlank(szLine)
    szLeft = strmid(szLine,0,nLeft);
    InsBufLine(hbuf, ln, "@szLeft@switch ( # )")    
    InsBufLine(hbuf, ln + 1, "@szLeft@" # "{")
    nSwitch = ask("������case�ĸ���")
    InsertMultiCaseProc(hbuf,szLeft,nSwitch)
    SearchForward()    
}

macro InsertMultiCaseProc(hbuf,szLeft,nSwitch)
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    ln = sel.lnFirst

    nIdx = 0
    if(nSwitch == 0)
    {
        hNewBuf = newbuf("clip")
        if(hNewBuf == hNil)
            return       
        SetCurrentBuf(hNewBuf)
        PasteBufLine (hNewBuf, 0)
        nLeftMax = 0
        lnMax = GetBufLineCount(hNewBuf )
        i = 0
        fIsEnd = 1
        while ( i < lnMax) 
        {
            szLine = GetBufLine(hNewBuf , i)
            //��ȥ��������ע�͵�����
            RetVal = SkipCommentFromString(szLine,fIsEnd)
            szLine = RetVal.szContent
            fIsEnd = RetVal.fIsEnd
//            nLeft = GetLeftBlank(szLine)
            //�Ӽ�������ȡ��caseֵ
            szLine = GetSwitchVar(szLine)
            if(strlen(szLine) != 0 )
            {
                ln = ln + 3
                InsBufLine(hbuf, ln - 1, "@szLeft@    " # "case @szLine@:")
                InsBufLine(hbuf, ln    , "@szLeft@    " # "    " # "#")
                InsBufLine(hbuf, ln + 1, "@szLeft@    " # "    " # "break;")
              }
              i = i + 1
        }
        closebuf(hNewBuf)
       }
       else
       {
        while(nIdx < nSwitch)
        {
            ln = ln + 3
            InsBufLine(hbuf, ln - 1, "@szLeft@    " # "case # :")
            InsBufLine(hbuf, ln    , "@szLeft@    " # "    " # "#")
            InsBufLine(hbuf, ln + 1, "@szLeft@    " # "    " # "break;")
            nIdx = nIdx + 1
        }
      }
    InsBufLine(hbuf, ln + 2, "@szLeft@    " # "default:")
    InsBufLine(hbuf, ln + 3, "@szLeft@    " # "    " # "#")
    InsBufLine(hbuf, ln + 4, "@szLeft@" # "}")
    SetWndSel(hwnd, sel)
    SearchForward()
}

macro GetSwitchVar(szLine)
{
    if( (szLine == "{") || (szLine == "}") )
    {
        return ""
    }
    ret = strstr(szLine,"#define" )
    if(ret != 0xffffffff)
    {
        szLine = strmid(szLine,ret + 8,strlen(szLine))
    }
    szLine = TrimLeft(szLine)
    nIdx = 0
    nLen = strlen(szLine)
    while( nIdx < nLen)
    {
        if((szLine[nIdx] == " ") || (szLine[nIdx] == ",") || (szLine[nIdx] == "="))
        {
            szLine = strmid(szLine,0,nIdx)
            return szLine
        }
        nIdx = nIdx + 1
    }
    return szLine
}

/*
macro SkipControlCharFromString(szLine)
{
   nLen = strlen(szLine)
   nIdx = 0
   newStr = ""
   while(nIdx < nLen - 1)
   {
       if(szLine[nIdx] == "\t")
       {
           newStr = cat(newStr,"    ")
       }
       else if(szLine[nIdx] < " ")
       {
           newStr = cat(newStr," ")           
       }
       else
       {
           newStr = cat(newStr," ")                      
       }
   }
}
*/
macro SkipCommentFromString(szLine,isCommentEnd)
{
    RetVal = ""
    fIsEnd = 1
    nLen = strlen(szLine)
    nIdx = 0
    while(nIdx < nLen )
    {
        //�����ǰ�п�ʼ���Ǳ�ע�ͣ���������ע�Ϳ�ʼ�ı��ǣ�ע�����ݸ�Ϊ�ո��
        if( (isCommentEnd == 0) || (szLine[nIdx] == "/" && szLine[nIdx+1] == "*"))
        {
            fIsEnd = 0
            while(nIdx < nLen )
            {
                if(szLine[nIdx] == "*" && szLine[nIdx+1] == "/")
                {
                    szLine[nIdx+1] = " "
                    szLine[nIdx] = " " 
                    nIdx = nIdx + 1 
                    fIsEnd  = 1
                    isCommentEnd = 1
                    break
                }
                szLine[nIdx] = " "
                
                //����ǵ����ڶ��������һ��Ҳ�϶�����ע����
//                if(nIdx == nLen -2 )
//                {
//                    szLine[nIdx + 1] = " "
//                }
                nIdx = nIdx + 1 
            }    
            
            //����Ѿ�������β��ֹ����
            if(nIdx == nLen)
            {
                break
            }
        }
        
        //�����������//��ע�͵�˵�����涼Ϊע��
        if(szLine[nIdx] == "/" && szLine[nIdx+1] == "/")
        {
            szLine = strmid(szLine,0,nIdx)
            break
        }
        nIdx = nIdx + 1                
    }
    RetVal.szContent = szLine;
    RetVal.fIsEnd = fIsEnd
    return RetVal
}

macro InsertDo()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    ln = sel.lnFirst
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        szLeft = CreateBlankString(sel.ichFirst)
        InsBufLine(hbuf, ln,szLeft)
        SetWndSel(hwnd,sel)
    }
    val = ExpandBraceLarge()
    szLeft = val.szLeft
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        PutBufLine(hbuf,ln+1, "@szLeft@    #")
    }
    PutBufLine(hbuf, sel.lnLast + val.nLineCount, "@szLeft@}while ( # );")    
//       SetBufIns (hbuf, sel.lnLast + val.nLineCount, strlen(szLeft)+8)
    InsBufLine(hbuf, ln, "@szLeft@do")    
    SearchForward()
}

macro InsertWhile()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    ln = sel.lnFirst
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        szLeft = CreateBlankString(sel.ichFirst)
        InsBufLine(hbuf, ln,szLeft)
        SetWndSel(hwnd,sel)
    }
    val = ExpandBraceLarge()
    szLeft = val.szLeft
    InsBufLine(hbuf, ln, "@szLeft@while ( # )")    
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        PutBufLine(hbuf,ln+2, "@szLeft@    #")
    }
    SetBufIns (hbuf, ln, strlen(szLeft)+7)
    SearchForward()
}

macro InsertFor()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    ln = sel.lnFirst
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        szLeft = CreateBlankString(sel.ichFirst)
        InsBufLine(hbuf, ln,szLeft)
        SetWndSel(hwnd,sel)
    }
    val = ExpandBraceLarge()
    szLeft = val.szLeft
    InsBufLine(hbuf, ln,"@szLeft@for ( # ; # ; # )")
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        PutBufLine(hbuf,ln+2, "@szLeft@    #")
    }
    sel.lnFirst = ln
    sel.lnLast = ln 
    sel.ichFirst = 0
    sel.ichLim = 0
    SetWndSel(hwnd, sel)
    SearchForward()
    szVar = ask("������ѭ������")
    PutBufLine(hbuf,ln, "@szLeft@for ( @szVar@ = # ; @szVar@ # ; @szVar@++ )")
    SearchForward()
}

macro InsertIf()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    ln = sel.lnFirst
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        szLeft = CreateBlankString(sel.ichFirst)
        InsBufLine(hbuf, ln,szLeft)
        SetWndSel(hwnd,sel)
    }
    val = ExpandBraceLarge()
    szLeft = val.szLeft
    InsBufLine(hbuf, ln, "@szLeft@if ( # )")    
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        PutBufLine(hbuf,ln+2, "@szLeft@    #")
    }
//       SetBufIns (hbuf, ln, strlen(szLeft)+4)
    SearchForward()
}

macro MergeString()
{
    hbuf = newbuf("clip")
    if(hbuf == hNil)
        return       
    SetCurrentBuf(hbuf)
    PasteBufLine (hbuf, 0)
    
    //�����������û�����ݣ��򷵻�
    lnMax = GetBufLineCount(hbuf )
    if( lnMax == 0 )
    {
        closebuf(hbuf)
        return ""
    }
    lnLast =  0
    if(lnMax > 1)
    {
        lnLast = lnMax - 1
         i = lnMax - 1
    }
    while ( i > 0) 
    {
        szLine = GetBufLine(hbuf , i-1)
        szLine = TrimLeft(szLine)
        nLen = strlen(szLine)
        if(szLine[nLen - 1] == "-")
        {
              szLine = strmid(szLine,0,nLen - 1)
        }
        nLen = strlen(szLine)
        if( (szLine[nLen - 1] != " ") && (AsciiFromChar (szLine[nLen - 1])  <= 160))
        {
              szLine = cat(szLine," ") 
        }
        SetBufIns (hbuf, lnLast, 0)
        SetBufSelText(hbuf,szLine)
        i = i - 1
    }
    szLine = GetBufLine(hbuf,lnLast)
    closebuf(hbuf)
    return szLine
}

macro ClearPrombleNo()
{
   SetReg ("PNO", "")
}

macro AddPromblemNo()
{
    szQuestion = ASK("Please Input problem number ");
    if(szQuestion == "#")
    {
       szQuestion = ""
       SetReg ("PNO", "")
    }
    else
    {
       SetReg ("PNO", szQuestion)
    }
    return szQuestion
}

/*
this macro convet selected  C++ coment block to C comment block 
for example:
  line "  // aaaaa "
  convert to  /* aaaaa */
*/
/*macro ComentCPPtoC()
{
    hwnd = GetCurrentWnd()
    lnFirst = GetWndSelLnFirst( hwnd )
    lnLast = GetWndSelLnLast( hwnd )

    lnCurrent = lnFirst
    fIsEnd = 1
    while ( lnCurrent <= lnLast )
    {
        fIsEnd = CmtCvtLine( lnCurrent,fIsEnd )
        lnCurrent = lnCurrent + 1;
    }
}*/

macro ComentCPPtoC()
{
    hwnd = GetCurrentWnd()
    hbuf = GetCurrentBuf()
    lnFirst = GetWndSelLnFirst( hwnd )
    lnCurrent = lnFirst
    lnLast = GetWndSelLnLast( hwnd )
    ch_comment = CharFromAscii(47)   
    isCommentEnd = 1
    isCommentContinue = 0
    while ( lnCurrent <= lnLast )
    {

        ich = 0
        szLine = GetBufLine(hbuf,lnCurrent)
        ilen = strlen(szLine)
        while ( ich < ilen )
        {
            if( (szLine[ich] != " ") && (szLine[ich] != "\t") )
            {
                break
            }
            ich = ich + 1
        }
        /*����ǿ��У���������*/
        if(ich == ilen)
        {         
            lnCurrent = lnCurrent + 1
            szOldLine = szLine
            continue 
        }
        
        /*�������ֻ��һ���ַ�*/
        if(ich > ilen - 2)
        {
            if( isCommentContinue == 1 )
            {
                szOldLine = cat(szOldLine,"  */")
                PutBufLine(hbuf,lnCurrent-1,szOldLine)
                isCommentContinue = 0
            }
            lnCurrent = lnCurrent + 1
            szOldLine = szLine
            continue 
        }       
        if( isCommentEnd == 1 )
        {
            /*���������ע������*/
            if(( szLine[ich]==ch_comment ) && (szLine[ich+1]==ch_comment))
            {
                
                /* ȥ���м�Ƕ�׵�ע�� */
                nIdx = ich + 2
                while ( nIdx < ilen -1 )
                {
                    if( (( szLine[nIdx] == "/" ) && (szLine[nIdx+1] == "*")||
                         ( szLine[nIdx] == "*" ) && (szLine[nIdx+1] == "/") )
                    {
                        szLine[nIdx] = " "
                        szLine[nIdx+1] = " "
                    }
                    nIdx = nIdx + 1
                }
                
                if( isCommentContinue == 1 )
                {
                    /* �����������ע��*/
                    szLine[ich] = " "
                    szLine[ich+1] = " "
                }
                else
                {
                    /*�������������ע��������ע�͵Ŀ�ʼ*/
                    szLine[ich] = "/"
                    szLine[ich+1] = "*"
                }
                if ( lnCurrent == lnLast )
                {
                    /*��������һ��������β��ӽ���ע�ͷ�*/
                    szLine = cat(szLine,"  */")
                    isCommentContinue = 0
                }
                /*���¸���*/
                PutBufLine(hbuf,lnCurrent,szLine)
                isCommentContinue = 1
                szOldLine = szLine
                lnCurrent = lnCurrent + 1
                continue 
            }
            else
            {   
                /*������е���ʼ����//ע��*/
                if( isCommentContinue == 1 )
                {
                    szOldLine = cat(szOldLine,"  */")
                    PutBufLine(hbuf,lnCurrent-1,szOldLine)
                    isCommentContinue = 0
                }
            }        
        }
        while ( ich < ilen - 1 )
        {
            //�����/*ע�����������ö�
            if( (isCommentEnd == 0) || (szLine[ich] == "/" && szLine[ich+1] == "*"))
            {
                isCommentEnd = 0
                while(ich < ilen - 1 )
                {
                    if(szLine[ich] == "*" && szLine[ich+1] == "/")
                    {
                        ich = ich + 1 
                        isCommentEnd = 1
                        break
                    }
                    ich = ich + 1 
                }
                if(ich >= ilen - 1)
                {
                    break
                }
            }
            
            if(( szLine[ich]==ch_comment ) && (szLine[ich+1]==ch_comment))
            {
                /* �����//ע��*/
                isCommentContinue = 1
                nIdx = ich
                //ȥ���ڼ��/* �� */ע�ͷ��������ע��Ƕ�״���
                while ( nIdx < ilen -1 )
                {
                    if( (( szLine[nIdx] == "/" ) && (szLine[nIdx+1] == "*")||
                         ( szLine[nIdx] == "*" ) && (szLine[nIdx+1] == "/") )
                    {
                        szLine[nIdx] = " "
                        szLine[nIdx+1] = " "
                    }
                    nIdx = nIdx + 1
                }
                szLine[ich+1] = "*"
                if( lnCurrent == lnLast )
                {
                    szLine = cat(szLine,"  */")
                }
                PutBufLine(hbuf,lnCurrent,szLine)
                break
            }
            ich = ich + 1
        }
        szOldLine = szLine
        lnCurrent = lnCurrent + 1
    }
}


macro ComentLine()
{
    hwnd = GetCurrentWnd()
    hbuf = GetCurrentBuf()
    lnFirst = GetWndSelLnFirst( hwnd )
    lnCurrent = lnFirst
    lnLast = GetWndSelLnLast( hwnd )
    lnOld = 0
    while ( lnCurrent <= lnLast )
    {
        szLine = GetBufLine(hbuf,lnCurrent)
        DelBufLine(hbuf,lnCurrent)
        nLeft = GetLeftBlank(szLine)
        szLeft = strmid(szLine,0,nLeft);
        szLine = TrimString(szLine)
        ilen = strlen(szLine)
        if(iLen == 0)
        {
            continue
        }
        nIdx = 0
        //ȥ���ڼ��/* �� */ע�ͷ��������ע��Ƕ�״���
        while ( nIdx < ilen -1 )
        {
            if( (( szLine[nIdx] == "/" ) && (szLine[nIdx+1] == "*")||
                 ( szLine[nIdx] == "*" ) && (szLine[nIdx+1] == "/") )
            {
                szLine[nIdx] = " "
                szLine[nIdx+1] = " "
            }
            nIdx = nIdx + 1
        }
        szLine = cat("/* ",szLine)
        lnOld = lnCurrent
        lnCurrent = CommentContent(hbuf,lnCurrent,szLeft,szLine,1)
        lnLast = lnCurrent - lnOld + lnLast
        lnCurrent = lnCurrent + 1
    }
}

macro CmtCvtLine(lnCurrent, isCommentEnd)
{
    hbuf = GetCurrentBuf()
    szLine = GetBufLine(hbuf,lnCurrent)
    ch_comment = CharFromAscii(47)   
    ich = 0
    ilen = strlen(szLine)
    
    fIsEnd = 1
    iIsComment = 0;
    
    while ( ich < ilen - 1 )
    {
        //�����/*ע�����������ö�
        if( (isCommentEnd == 0) || (szLine[ich] == "/" && szLine[ich+1] == "*"))
        {
            fIsEnd = 0
            while(ich < ilen - 1 )
            {
                if(szLine[ich] == "*" && szLine[ich+1] == "/")
                {
                    ich = ich + 1 
                    fIsEnd  = 1
                    isCommentEnd = 1
                    break
                }
                ich = ich + 1 
            }
            if(ich >= ilen - 1)
            {
                break
            }
        }
        if(( szLine[ich]==ch_comment ) && (szLine[ich+1]==ch_comment))
        {
            nIdx = ich
            while ( nIdx < ilen -1 )
            {
                if( (( szLine[nIdx] == "/" ) && (szLine[nIdx+1] == "*")||
                     ( szLine[nIdx] == "*" ) && (szLine[nIdx+1] == "/") )
                {
                    szLine[nIdx] = " "
                    szLine[nIdx+1] = " "
                }
                nIdx = nIdx + 1
            }
            szLine[ich+1] = "*"
            szLine = cat(szLine,"  */")
            DelBufLine(hbuf,lnCurrent)
            InsBufLine(hbuf,lnCurrent,szLine)
            return fIsEnd
        }
        ich = ich + 1
    }
    return fIsEnd
}

macro GetFileNameExt(sz)
{
    i = 1
    j = 0
    szName = sz
    iLen = strlen(sz)
    if(iLen == 0)
      return ""
    while( i <= iLen)
    {
      if(sz[iLen-i] == ".")
      {
         j = iLen-i 
         szExt = strmid(sz,j + 1,iLen)
         return szExt
      }
      i = i + 1
    }
    return ""
}

macro GetFileNameNoExt(sz)
{
    i = 1
    szName = sz
    iLen = strlen(sz)
    j = iLen 
    if(iLen == 0)
      return ""
    while( i <= iLen)
    {
      if(sz[iLen-i] == ".")
      {
         j = iLen-i 
      }
      if( sz[iLen-i] == "\\" )
      {
         szName = strmid(sz,iLen-i+1,j)
         return szName
      }
      i = i + 1
    }
    szName = strmid(sz,0,j)
    return szName
}

macro GetFileName(sz)
{
    i = 1
    szName = sz
    iLen = strlen(sz)
    if(iLen == 0)
      return ""
    while( i <= iLen)
    {
      if(sz[iLen-i] == "\\")
      {
        szName = strmid(sz,iLen-i+1,iLen)
        break
      }
      i = i + 1
    }
    return szName
}

macro InsIfdef()
{
    sz = Ask("Enter #ifdef condition:")
    if (sz != "")
        IfdefStr(sz);
}

macro InsIfndef()
{
    sz = Ask("Enter #ifndef condition:")
    if (sz != "")
        IfndefStr(sz);
}

macro InsertCPP(hbuf,ln)
{
    InsBufLine(hbuf, ln, "")
    InsBufLine(hbuf, ln, "#endif /* __cplusplus */")
    InsBufLine(hbuf, ln, "#endif")
    InsBufLine(hbuf, ln, "extern \"C\"{")
    InsBufLine(hbuf, ln, "#if __cplusplus")
    InsBufLine(hbuf, ln, "#ifdef __cplusplus")
    InsBufLine(hbuf, ln, "")
    
    iTotalLn = GetBufLineCount (hbuf)            
    InsBufLine(hbuf, iTotalLn, "")
    InsBufLine(hbuf, iTotalLn, "#endif /* __cplusplus */")
    InsBufLine(hbuf, iTotalLn, "#endif")
    InsBufLine(hbuf, iTotalLn, "}")
    InsBufLine(hbuf, iTotalLn, "#if __cplusplus")
    InsBufLine(hbuf, iTotalLn, "#ifdef __cplusplus")
    InsBufLine(hbuf, iTotalLn, "")
}

macro ReviseCommentProc(hbuf,ln,szCmd,szMyName,szLine1)
{
    if (szCmd == "ap")
    {   
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day
        
        DelBufLine(hbuf, ln)
        szQuestion = AddPromblemNo()
        InsBufLine(hbuf, ln, "@szLine1@/* �� �� ��: @szQuestion@     �޸���:@szMyName@,   ʱ��:@sz@/@sz1@/@sz3@ ");
        szContent = Ask("�޸�ԭ��")
        szLeft = cat(szLine1,"   �޸�ԭ��: ");
        if(strlen(szLeft) > 70)
        {
            Msg("The right margine is small, Please use a new line")
            stop 
        }
        ln = CommentContent(hbuf,ln + 1,szLeft,szContent,1)
        return
    }
    else if (szCmd == "ab")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day
        
        DelBufLine(hbuf, ln)
        szQuestion = GetReg ("PNO")
        if(strlen(szQuestion)>0)
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Added by @szMyName@, @sz@/@sz1@/@sz3@   ���ⵥ��:@szQuestion@*/");
        }
        else
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Added by @szMyName@, @sz@/@sz1@/@sz3@ */");        
        }
        return
    }
    else if (szCmd == "ae")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day

        DelBufLine(hbuf, ln)
        InsBufLine(hbuf, ln, "@szLine1@/* END:   Added by @szMyName@, @sz@/@sz1@/@sz3@ */");
        return
    }
    else if (szCmd == "db")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day

        DelBufLine(hbuf, ln)
        szQuestion = GetReg ("PNO")
        if(strlen(szQuestion) > 0)
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Deleted by @szMyName@, @sz@/@sz1@/@sz3@   ���ⵥ��:@szQuestion@*/");
        }
        else
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Deleted by @szMyName@, @sz@/@sz1@/@sz3@ */");
        }
        
        return
    }
    else if (szCmd == "de")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day

        DelBufLine(hbuf, ln + 0)
        InsBufLine(hbuf, ln, "@szLine1@/* END: Deleted by @szMyName@, @sz@/@sz1@/@sz3@ */");
        return
    }
    else if (szCmd == "mb")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day

        DelBufLine(hbuf, ln)
        szQuestion = GetReg ("PNO")
            if(strlen(szQuestion) > 0)
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Modified by @szMyName@, @sz@/@sz1@/@sz3@   ���ⵥ��:@szQuestion@*/");
        }
        else
        {
            InsBufLine(hbuf, ln, "@szLine1@/* BEGIN: Modified by @szMyName@, @sz@/@sz1@/@sz3@ */");
        }
        return
    }
    else if (szCmd == "me")
    {
        SysTime = GetSysTime(1)
        sz=SysTime.Year
        sz1=SysTime.month
        sz3=SysTime.day
        MSG(SysTime)

        DelBufLine(hbuf, ln)
        InsBufLine(hbuf, ln, "@szLine1@/* END:   Modified by @szMyName@, @sz@/@sz1@/@sz3@ */");
        return
    }
}
macro InsertReviseAdd()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    lnMax = GetBufLineCount(hbuf)
    language = getreg(LANGUAGE)
    if(language != 1)
    {
        language = 0
    }
    szMyName = getreg(MYNAME)
    if(strlen( szMyName ) == 0)
    {
        szMyName = Ask("Enter your name:")
        setreg(MYNAME, szMyName)
    }
    SysTime = GetSysTime(1)
    sz=SysTime.Year
    sz1=SysTime.month
    sz3=SysTime.day
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        szLeft = CreateBlankString(sel.ichFirst)
    }
    else
    {
        szLine = GetBufLine( hbuf, sel.lnFirst )    
        nLeft = GetLeftBlank(szLine)
        szLeft = strmid(szLine,0,nLeft);
    }
    szQuestion = GetReg ("PNO")
    if(strlen(szQuestion)>0)
    {
        InsBufLine(hbuf, sel.lnFirst, "@szLeft@/* BEGIN: Added by @szMyName@, @sz@/@sz1@/@sz3@   PN:@szQuestion@ */");
    }
    else
    {
        InsBufLine(hbuf, sel.lnFirst, "@szLeft@/* BEGIN: Added by @szMyName@, @sz@/@sz1@/@sz3@ */");        
    }

    if(sel.lnLast < lnMax - 1)
    {
        InsBufLine(hbuf, sel.lnLast + 2, "@szLeft@/* END:   Added by @szMyName@, @sz@/@sz1@/@sz3@   PN:@szQuestion@ */");            
    }
    else
    {
        AppendBufLine(hbuf, "@szLeft@/* END:   Added by @szMyName@, @sz@/@sz1@/@sz3@ */");                        
    }
    SetBufIns(hbuf,sel.lnFirst + 1,strlen(szLeft))
}

macro InsertReviseDel()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    lnMax = GetBufLineCount(hbuf)
    language = getreg(LANGUAGE)
    if(language != 1)
    {
        language = 0
    }
    szMyName = getreg(MYNAME)
    if(strlen( szMyName ) == 0)
    {
        szMyName = Ask("Enter your name:")
        setreg(MYNAME, szMyName)
    }
    SysTime = GetSysTime(1)
    sz=SysTime.Year
    sz1=SysTime.month
    sz3=SysTime.day
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        szLeft = CreateBlankString(sel.ichFirst)
    }
    else
    {
        szLine = GetBufLine( hbuf, sel.lnFirst )    
        nLeft = GetLeftBlank(szLine)
        szLeft = strmid(szLine,0,nLeft);
    }
    szQuestion = GetReg ("PNO")
    if(strlen(szQuestion)>0)
    {
        InsBufLine(hbuf, sel.lnFirst, "@szLeft@/* BEGIN: Deleted by @szMyName@, @sz@/@sz1@/@sz3@   PN:@szQuestion@ */");
    }
    else
    {
        InsBufLine(hbuf, sel.lnFirst, "@szLeft@/* BEGIN: Deleted by @szMyName@, @sz@/@sz1@/@sz3@ */");        
    }

    if(sel.lnLast < lnMax - 1)
    {
        InsBufLine(hbuf, sel.lnLast + 2, "@szLeft@/* END:   Deleted by @szMyName@, @sz@/@sz1@/@sz3@   PN:@szQuestion@ */");            
    }
    else
    {
        AppendBufLine(hbuf, "@szLeft@/* END:   Deleted by @szMyName@, @sz@/@sz1@/@sz3@ */");                        
    }
    SetBufIns(hbuf,sel.lnFirst + 1,strlen(szLeft))
}

macro InsertReviseMod()
{
    hwnd = GetCurrentWnd()
    sel = GetWndSel(hwnd)
    hbuf = GetCurrentBuf()
    lnMax = GetBufLineCount(hbuf)
    language = getreg(LANGUAGE)
    if(language != 1)
    {
        language = 0
    }
    szMyName = getreg(MYNAME)
    if(strlen( szMyName ) == 0)
    {
        szMyName = Ask("Enter your name:")
        setreg(MYNAME, szMyName)
    }
    SysTime = GetSysTime(1)
    sz=SysTime.Year
    sz1=SysTime.month
    sz3=SysTime.day
    if(sel.lnFirst == sel.lnLast && sel.ichFirst == sel.ichLim)
    {
        szLeft = CreateBlankString(sel.ichFirst)
    }
    else
    {
        szLine = GetBufLine( hbuf, sel.lnFirst )    
        nLeft = GetLeftBlank(szLine)
        szLeft = strmid(szLine,0,nLeft);
    }
    szQuestion = GetReg ("PNO")
    if(strlen(szQuestion)>0)
    {
        InsBufLine(hbuf, sel.lnFirst, "@szLeft@/* BEGIN: Modified by @szMyName@, @sz@/@sz1@/@sz3@   PN:@szQuestion@ */");
    }
    else
    {
        InsBufLine(hbuf, sel.lnFirst, "@szLeft@/* BEGIN: Modified by @szMyName@, @sz@/@sz1@/@sz3@ */");        
    }

    if(sel.lnLast < lnMax - 1)
    {
        InsBufLine(hbuf, sel.lnLast + 2, "@szLeft@/* END:   Modified by @szMyName@, @sz@/@sz1@/@sz3@   PN:@szQuestion@ */");            
    }
    else
    {
        AppendBufLine(hbuf, "@szLeft@/* END:   Modified by @szMyName@, @sz@/@sz1@/@sz3@ */");                        
    }
    SetBufIns(hbuf,sel.lnFirst + 1,strlen(szLeft))
}

// Wrap ifdef <sz> .. endif around the current selection
macro IfdefStr(sz)
{
    hwnd = GetCurrentWnd()
    lnFirst = GetWndSelLnFirst(hwnd)
    lnLast = GetWndSelLnLast(hwnd)
    hbuf = GetCurrentBuf()
    lnMax = GetBufLineCount(hbuf)
    if(lnMax != 0)
    {
        szLine = GetBufLine( hbuf, lnFirst )    
    }
    nLeft = GetLeftBlank(szLine)
    szLeft = strmid(szLine,0,nLeft);
     
    hbuf = GetCurrentBuf()
    if(lnLast + 1 < lnMax)
    {
        InsBufLine(hbuf, lnLast+1, "@szLeft@#endif /* @sz@ */")
    }
    else if(lnLast + 1 == lnMax)
    {
        AppendBufLine(hbuf, "@szLeft@#endif /* @sz@ */")
    }
    else 
    {
        AppendBufLine(hbuf, "")
        AppendBufLine(hbuf, "@szLeft@#endif /* @sz@ */")
    }    
    InsBufLine(hbuf, lnFirst, "@szLeft@#ifdef @sz@")
    SetBufIns(hbuf,lnFirst + 1,strlen(szLeft))
}
macro IfndefStr(sz)
{
    hwnd = GetCurrentWnd()
    lnFirst = GetWndSelLnFirst(hwnd)
    lnLast = GetWndSelLnLast(hwnd)
    hbuf = GetCurrentBuf()
    lnMax = GetBufLineCount(hbuf)
    if(lnMax != 0)
    {
        szLine = GetBufLine( hbuf, lnFirst )    
    }
    nLeft = GetLeftBlank(szLine)
    szLeft = strmid(szLine,0,nLeft);
     
    hbuf = GetCurrentBuf()
    if(lnLast + 1 < lnMax)
    {
        InsBufLine(hbuf, lnLast+1, "@szLeft@#endif /* @sz@ */")
    }
    else if(lnLast + 1 == lnMax)
    {
        AppendBufLine(hbuf, "@szLeft@#endif /* @sz@ */")
    }
    else 
    {
        AppendBufLine(hbuf, "")
        AppendBufLine(hbuf, "@szLeft@#endif /* @sz@ */")
    }    
    InsBufLine(hbuf, lnFirst, "@szLeft@#ifndef @sz@")
    SetBufIns(hbuf,lnFirst + 1,strlen(szLeft))
}


macro InsertPredefIf()
{
    sz = Ask("Enter #if condition:")
    PredefIfStr(sz)
}

macro PredefIfStr(sz)
{
    hwnd = GetCurrentWnd()
    lnFirst = GetWndSelLnFirst(hwnd)
    lnLast = GetWndSelLnLast(hwnd)
    hbuf = GetCurrentBuf()
    lnMax = GetBufLineCount(hbuf)
    if(lnMax != 0)
    {
        szLine = GetBufLine( hbuf, lnFirst )    
    }
    nLeft = GetLeftBlank(szLine)
    szLeft = strmid(szLine,0,nLeft);
     
    hbuf = GetCurrentBuf()
    if(lnLast + 1 < lnMax)
    {
        InsBufLine(hbuf, lnLast+1, "@szLeft@#endif /* #if @sz@ */")
    }
    else if(lnLast + 1 == lnMax)
    {
        AppendBufLine(hbuf, "@szLeft@#endif /* #if @sz@ */")
    }
    else 
    {
        AppendBufLine(hbuf, "")
        AppendBufLine(hbuf, "@szLeft@#endif /* #if @sz@ */")
    }    
    InsBufLine(hbuf, lnFirst, "@szLeft@#if  @sz@")
    SetBufIns(hbuf,lnFirst + 1,strlen(szLeft))
}

macro HeadIfdefStr(sz)
{
    hwnd = GetCurrentWnd()
    lnFirst = GetWndSelLnFirst(hwnd)
    hbuf = GetCurrentBuf()
    InsBufLine(hbuf, lnFirst, "")
    InsBufLine(hbuf, lnFirst, "#define @sz@")
    InsBufLine(hbuf, lnFirst, "#ifndef @sz@")
    iTotalLn = GetBufLineCount (hbuf)                
    InsBufLine(hbuf, iTotalLn, "#endif /* @sz@ */")
    InsBufLine(hbuf, iTotalLn, "")
}

macro GetSysTime(a)
{
    //��sidateȡ��ʱ��
    RunCmd ("sidate")
    SysTime=""
    SysTime.Year=getreg(Year)
    if(strlen(SysTime.Year)==0)
    {
        setreg(Year,"2002")
        setreg(Month,"05")
        setreg(Day,"02")
        SysTime.Year="2002"
        SysTime.month="05"
        SysTime.day="20"
        SysTime.Date="2002��05��20��"
    }
    else
    {
        SysTime.Month=getreg(Month)
        SysTime.Day=getreg(Day)
        SysTime.Hour =getreg(Hour)
        SysTime.Date=getreg(Date)
        MSG(SysTime)
   /*         SysTime.Date=cat(SysTime.Year,"��")
        SysTime.Date=cat(SysTime.Date,SysTime.Month)
        SysTime.Date=cat(SysTime.Date,"��")
        SysTime.Date=cat(SysTime.Date,SysTime.Day)
        SysTime.Date=cat(SysTime.Date,"��")*/
    }
    return SysTime
}

macro HeaderFileCreate()
{
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    sel = GetWndSel(hwnd)
    hbuf = GetWndBuf(hwnd)
    language = getreg(LANGUAGE)
    if(language != 1)
    {
        language = 0
    }
    szMyName = getreg(MYNAME)
    if(strlen( szMyName ) == 0)
    {
        szMyName = Ask("Enter your name:")
        setreg(MYNAME, szMyName)
    }

   CreateFunctionDef(hbuf,szMyName,language)
}

macro FunctionHeaderCreate()
{
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    sel = GetWndSel(hwnd)
    ln = sel.lnFirst
    hbuf = GetWndBuf(hwnd)
    language = getreg(LANGUAGE)
    if(language != 1)
    {
        language = 0
    }
    szMyName = getreg(MYNAME)
    if(strlen( szMyName ) == 0)
    {
        szMyName = Ask("Enter your name:")
        setreg(MYNAME, szMyName)
    }
    nVer = GetVersion()
    lnMax = GetBufLineCount(hbuf)
    if(ln != lnMax)
    {
        szNextLine = GetBufLine(hbuf,ln)
        if( (strstr(szNextLine,"(") != 0xffffffff) || (nVer != 2 ))
        {
            symbol = GetCurSymbol()
            if(strlen(symbol) != 0)
            {  
                if(language == 0)
                {
                    FuncHeadCommentCN(hbuf, ln, symbol, szMyName,0)
                }
                else
                {                
                    FuncHeadCommentEN(hbuf, ln, symbol, szMyName,0)
                }
                return
            }
        }
    }
    if(language == 0 )
    {
        szFuncName = Ask("�����뺯������:")
            FuncHeadCommentCN(hbuf, ln, szFuncName, szMyName, 1)
    }
    else
    {
        szFuncName = Ask("Please input function name")
           FuncHeadCommentEN(hbuf, ln, szFuncName, szMyName, 1)
    
    }
}

macro GetVersion()
{
   Record = GetProgramInfo ()
   return Record.versionMajor
}

macro GetProgramInfo ()
{   
    Record = ""
    Record.versionMajor     = 2
    Record.versionMinor    = 1
    return Record
}

macro FileHeaderCreate()
{
    hwnd = GetCurrentWnd()
    if (hwnd == 0)
        stop
    ln = 0
    hbuf = GetWndBuf(hwnd)
    language = getreg(LANGUAGE)
    if(language != 1)
    {
        language = 0
    }
    szMyName = getreg(MYNAME)
    if(strlen( szMyName ) == 0)
    {
        szMyName = Ask("Enter your name:")
        setreg(MYNAME, szMyName)
    }
       SetBufIns (hbuf, 0, 0)
    if(language == 0)
    {
        InsertFileHeaderCN( hbuf,ln, szMyName,"" )
    }
    else
    {
        InsertFileHeaderEN( hbuf,ln, szMyName,"" )
    }
}

macro TabSize()
{
	return  8;
}
macro TabSpace(cnt)
{
	spaces = ""
	while (cnt) {
		spaces = "@spaces@        "
		//MSG("[" # spaces # "]")
		cnt = cnt - 1
	}
	return spaces
}
macro GetPaddingSpace(lb)
{
	cnt = 0
	while (1) {
		if (lb[cnt] != " ")
			break;
		cnt = cnt + 1
	}
	return cnt / TabSize()
}

macro _MyGetSymbol(symbol)
{
	//MSG(symbol)
	hsyml = SymbolChildren(symbol)
	cchild = SymListCount(hsyml)
	//MSG("cchild=" # cchild)
	ichild = 0
	parent_symbol = ""
	while (ichild < cchild)
	{
		childsym = SymListItem(hsyml, ichild)
		//MSG(childsym.type)
		if (childsym.type == "Variable")
		{
			if (SymbolLeafName(childsym) == "axxxxxxbbooo")
			{
				//MSG(childsym)
				parent_symbol = SymbolParent(childsym)
				break
			}
		}
		else if (childsym.type == "Code Block")
		{
			parent_symbol = _MyGetSymbol(childsym)
			if (strlen(parent_symbol) > 0)
				break
		}
		ichild = ichild + 1
	}
	SymListFree(hsyml)
	return  parent_symbol;
}

macro MYGetCurSymbol(hbuf, ln)
{
	InsBufLine(hbuf, ln, "int axxxxxxbbooo;")
	psymbole = _MyGetSymbol(GetSymbolLocation(GetCurSymbol()))
	DelBufLine(hbuf, ln)
	//MSG(psymbole)
	return  psymbole
}


macro MyGetBufFile(hbuf)
{
	//InsBufLine(hbuf, 0, "#define __file_head__")
	//symbol = GetSymbolLocationFromLn(hbuf, 0)
	//file_path = symbol.file

	file_path = GetBufName(hbuf)
	total_index = strlen(file_path)
	now_index = total_index - 1
	
	while (file_path[now_index] != "\\")
	{
		now_index = now_index - 1;
	}
	
	//DelBufLine(hbuf, 0)
	file_name = strmid(file_path, now_index + 1, total_index)
	return file_name
}


