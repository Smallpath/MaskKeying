#include "Skeleton.h"

static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	suites.ANSICallbacksSuite1()->sprintf(	out_data->return_msg,
											"%s v%d.%d\r%s",
											STR(StrID_Name), 
											MAJOR_VERSION, 
											MINOR_VERSION, 
											STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err 
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err				err			= PF_Err_NONE;
	PF_Handle			globH		= NULL;
	my_global_dataP		globP		= NULL;

	AEGP_SuiteHandler		suites(in_data->pica_basicP);
	out_data->my_version = PF_VERSION(	MAJOR_VERSION, 
										MINOR_VERSION,
										BUG_VERSION, 
										STAGE_VERSION, 
										BUILD_VERSION);

	out_data->out_flags =  PF_OutFlag_DEEP_COLOR_AWARE;	// just 16bpc, not 32bpc

	globH	=	suites.HandleSuite1()->host_new_handle(sizeof(my_global_data));
	globH	=	suites.HandleSuite1()->host_new_handle(sizeof(my_global_data));
	
	if (globH) {
		globP = reinterpret_cast<my_global_dataP>(suites.HandleSuite1()->host_lock_handle(globH));
		if (globP) {
			globP->initializedB 	= TRUE;

			if (in_data->appl_id != 'PrMr') {
				ERR(suites.UtilitySuite3()->AEGP_RegisterWithAEGP(NULL, STR(StrID_Name), &globP->my_id));
			}
				if (!err){
					out_data->global_data 	= globH;
			}
		}
		suites.HandleSuite1()->host_unlock_handle(globH);
	} else	{
		err = PF_Err_INTERNAL_STRUCT_DAMAGED;
	}

	return err;
}

static PF_Err 
ParamsSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err		err		= PF_Err_NONE;
	PF_ParamDef	def;	

	AEFX_CLR_STRUCT(def);
	def.param_type = PF_Param_PATH;
	def.uu.id = SKELETON_Flood_Seed_DISK_ID;
	PF_STRCPY(def.name, STR(StrID_Flood_Seed_Param_Name)); 
	def.u.path_d.dephault	= 0;
	PF_ADD_PARAM(in_data, -1, &def);
	AEFX_CLR_STRUCT(def);
	PF_ADD_255_SLIDER(STR(StrID_BG_Param_Name),128,SKELETON_BG_DISK_ID);
	AEFX_CLR_STRUCT(def);
	PF_ADD_BUTTON(STR(StrID_Set_Mask_Param_Name),"Reset Mask currently",0,PF_ParamFlag_SUPERVISE,SKELETON_Set_Mask_DISK_ID);
	AEFX_CLR_STRUCT(def);
	
	
	out_data->num_params = SKELETON_NUM_PARAMS;
	return err;
}



PF_Pixel8 *samplePos(PF_EffectWorld &def, int x, int y){
	return (PF_Pixel8*)((char*)def.data +(y * def.rowbytes) +(x * sizeof(PF_Pixel8)));
}

boolean IsPosValid(int x,int y ,int xL,int yL){
		if(x>=0&&x<xL&&y>=0&&y<yL){
				return true;
		}else{
				return false;
		}

}

boolean IsBlack(int gray,int threshold,int type){

	if(gray<=threshold){    //�Ҷȵ��ڷ�ֵ,�򷵻�false������alpha��Ϊ0
		if(type==0){
			return false;
		}else{
			return true;
		}
	}else{
		if(type==0){
			return true;
		}else{
			return false;
		}
	}
}

boolean IsColorWaitToDel(PF_EffectWorld &def,int x,int y,int xL,int yL,int isDel,int threshold,int type){
	
	if(IsPosValid(x,y,xL,yL)){
		PF_Pixel8 *old_color = samplePos(def,x,y);
		int gray = (old_color->red*38 + old_color->green*75 + old_color->blue*15) >> 7;
		if(old_color->alpha !=0&&IsBlack(gray,threshold,type)){     //ɾ���˴�alpha��������:alpha��Ϊ0�һҶȵ��ڷ�ֵ
			if (isDel == 1)
			old_color->alpha =0;
			return true;
		}else{
			return false;
		}
	}else{
		return false;
	}

}

link stack = 0;

void push(int xx, int yy) {
    link new_node;
    new_node = (stack_list * ) malloc(sizeof(stack_list));
    new_node ->x = xx;
    new_node ->y = yy;
    new_node ->next = stack;
    stack = new_node;
}

//pop an element
void pop(int & xx, int & yy) {
    link top;
    top = stack;
    xx = stack -> x;
    yy = stack -> y;
    stack = stack -> next;
    free(top);
}


//fill the plot
void fill(PF_EffectWorld &def,int x,int y,int threshold) {
    int x0, y0, xl, xr, xlold, xrold; /*x0,y0�������x,y��ֵ,xl��¼x������ֵ,xr��¼x������ֵ*/
    int go = 0, go2 = 0;
	int xL = def.width;
	int yL = def.height;
    int i = 0;
	int type = 0;
	PF_Pixel8 *old_color = samplePos(def,x,y);
	int gray = (old_color->red*38 + old_color->green*75 + old_color->blue*15) >> 7;
	if(gray>threshold) type=0;
	else  type=1;
    push(x, y);				//����������ջ
    while (stack != 0)		//���ջ������ѭ����stack==0��ʾջ��
    {
        go = 0;				//go ֻ��һ�����
        pop(x, y);
		IsColorWaitToDel(def,x,y,xL,yL,1,threshold,type);
        x0 = x + 1;
        while (IsColorWaitToDel(def,x0,y,xL,yL,1,threshold,type)) //fill right ����ұ�
        {
            x0 = x0 + 1;
        }
        xr = x0 - 1;		//��¼����ֵ
        xrold = xr;			//�ټ�¼һ������ֵ���Ա�����
        x0 = x - 1;
        while (IsColorWaitToDel(def,x0,y,xL,yL,1,threshold,type)) //fill right ������
        {
            x0 = x0 - 1;
        }
        xl = x0 + 1;		//��¼����ֵ
        xlold = xl;			//�ټ�¼һ������ֵ���Ա�����
        y0 = y + 1;			//go up ������һ��ɨ����
        go2 = 0;			//go2 Ҳֻ��һ��������ǵı���
        while (xr > xl && go == 0) //������һ���ߵ�����ֵ,����¼Ϊxr
        {
            if (!IsColorWaitToDel(def,xr,y0,xL,yL,0,threshold,type)) {
                xr = xr - 1;
            } else {
                go = 1;		//��go=1���ִ�еĻ���˵���ҵ�������ֵ������while�е�go==0���жϲ��˳�while
            }
        }
        while (xl < xr && go2 == 0) //������һ���ߵ�����ֵ������¼Ϊxl
        {
            if (!IsColorWaitToDel(def,xl,y0,xL,yL,0,threshold,type))
                xl = xl + 1;
            else
                go2 = 1;	//go2=1���ִ�о�˵���ҵ�������ֵ�����ڴ�while�е�go2==0���˳�while
        }
        if (go == 1 && go2 == 1) //����ҵ�������ֵ������ֵ����ִ����������
        {
							//OutputDebugString("Down 1");
            push(xr, y0);	//�Ƚ���һ�����ϵ����ҵ���Ϊ���ӵ���ջ
            for (i = xl; i < xr; i++)	//����������ѭ������ÿ��������������һ�����ӵ���ջ
            {
                if (IsColorWaitToDel(def,i,y0,xL,yL,0,threshold,type))	//������Ǳ߽�㣬ʲôҲ����
                {} else if (IsColorWaitToDel(def,i-1,y0,xL,yL,0,threshold,type))		//����Ǳ߽�㣬������ߵĵ��ǲ��Ǳ߽�㣬������ǣ�����ջ
                {
                    push(i - 1, y0);
                }
            }

        }

        y0 = y - 1; //go down;//������һ��ɨ����
        go = 0;
        go2 = 0;
        xl = xlold; //��ԭ��������
        xr = xrold;

        while (xr > xl && go == 0) //����һ���ߵ�����
        {
            if (IsColorWaitToDel(def,xr,y0,xL,yL,0,threshold,type)) {
                go = 1;
            } else {
                xr--;
            }

        }
        while (xl < xr && go2 == 0) //����һ���ߵ�����
        {
            if (IsColorWaitToDel(def,xl,y0,xL,yL,0,threshold,type))
                go2 = 1;
            else
                xl++;
        }
        if (go == 1 && go2 == 1) //����ҵ���������ң���ִ��
        {
            push(xr, y0);
            for (i = xl; i <= xr; i++) {
                if (IsColorWaitToDel(def,i,y0,xL,yL,0,threshold,type)) {} else if (IsColorWaitToDel(def,i-1,y0,xL,yL,0,threshold,type) != 4) {
                    push(i - 1, y0);
                }
            }
        }
    }
}


static PF_Err
UserChangedParam(
	PF_InData						*in_data,
	PF_OutData						*out_data,
	PF_ParamDef						*params[],
	PF_LayerDef						*outputP,
	const PF_UserChangedParamExtra	*which_hitP)
{
	PF_Err				err					= PF_Err_NONE;
	my_global_dataP		globP				= reinterpret_cast<my_global_dataP>(DH(out_data->global_data));
	AEGP_SuiteHandler	suites(in_data->pica_basicP);
	if (which_hitP->param_index == SKELETON_Set_Mask)
	{

			PF_MaskSuite1		*msP		= NULL;
			PF_EffectWorld		*inputP 		= &params[0]->u.ld;
			PF_EffectWorld		*outputPP 		= outputP;
			PF_PathOutlinePtr	maskP		= 0;
			A_long				mask_idL	= params[SKELETON_Flood_Seed]->u.path_d.path_id;
			PF_ProgPtr			progPtr	=in_data->effect_ref;
			A_long				fxIndex;	//based on 0;

			ERR(AEFX_AcquireSuite(in_data,
								  out_data,
								  kPF_MaskSuite,
								  kPF_MaskSuiteVersion1,
								  0,
								  reinterpret_cast<void**>(&msP)));

			ERR(suites.PathQuerySuite1()->PF_CheckoutPath(	in_data->effect_ref, 
																mask_idL,
																in_data->current_time,
																in_data->time_step,
																in_data->time_scale,
																&maskP));
			if(maskP!=0&&mask_idL){
					AEGP_EffectRefH		effectRef		=NULL;
					AEGP_StreamRefH		streamA			=NULL;
					AEGP_StreamRefH		parentStream	=NULL;
					AEGP_LayerIDVal		layer_id;
					A_long				layer_index;
					A_Time				timeT			= {0,1};
					AEGP_StreamValue2	value;
					ERR(suites.PFInterfaceSuite1()->AEGP_GetNewEffectForEffect(globP->my_id,in_data->effect_ref,&effectRef));

					ERR(suites.StreamSuite4()->AEGP_GetNewEffectStreamByIndex(globP->my_id, effectRef,1 , &streamA));   
                    ERR(suites.DynamicStreamSuite4()->AEGP_GetNewParentStreamRef( globP->my_id, streamA, &parentStream));   
                    ERR(suites.DynamicStreamSuite4()->AEGP_GetStreamIndexInParent( parentStream, &fxIndex)); 
					ERR(suites.StreamSuite4()->AEGP_GetNewStreamValue(globP->my_id, streamA, AEGP_LTimeMode_LayerTime, &timeT, TRUE, &value));
					layer_id = value.val.layer_id;
					AEGP_LayerH effect_layer_handle = NULL;
					ERR(suites.PFInterfaceSuite1()->AEGP_GetEffectLayer(in_data->effect_ref, &effect_layer_handle));
					ERR(suites.LayerSuite8()->AEGP_GetLayerIndex(effect_layer_handle,&layer_index));

			  char buffer[700];
			  sprintf_s(buffer,"var comp=app.project.activeItem;var first=%d+1,second=%d+1;v1x=v4x=v5x=-1;v1y=v2y=-1;v2x=v3x=comp.width+1;v3y=v4y=comp.height+1;v5y=0;",layer_index,fxIndex);
			  strcat_s(buffer,"var time = comp.time;\
var prePropValue = comp.layer(first).property('ADBE Effect Parade')(second)(1).value;\
var prop=comp.layer(first).property('ADBE Mask Parade')(prePropValue)('ADBE Mask Shape');\
var thisValue = prop.valueAtTime(time,false);\
thisValue.vertices= [[v1x-1,v1y-1],[v2x+1,v2y-1],[v3x+1,v3y+1],[v4x-1,v4y+1],[v5x-1,v5y]];\
thisValue.inTangents=[[0,0],[0,0],[0,0],[0,0],[0,0]];\
thisValue.outTangents=[[0,0],[0,0],[0,0],[0,0],[0,0]];\
thisValue.closed = false;\
prop.setValueAtTime (time,thisValue);");
			 // OutputDebugString(buffer);
			  ERR(suites.UtilitySuite6()->AEGP_ExecuteScript(NULL,buffer,FALSE,NULL,NULL));
			  ERR(suites.StreamSuite4()->AEGP_DisposeStreamValue(&value));
			  if(parentStream)
			  ERR(suites.StreamSuite4()->AEGP_DisposeStream(parentStream));
			  if(streamA)
			  ERR(suites.StreamSuite4()->AEGP_DisposeStream(streamA));
			  if(effectRef)
						ERR(suites.EffectSuite3()->AEGP_DisposeEffect(effectRef));
			}
			ERR(suites.PathQuerySuite1()->PF_CheckinPath(	in_data->effect_ref, 
														mask_idL,
														FALSE, 
														maskP));
			if (msP){
				ERR(AEFX_ReleaseSuite(in_data,
									   out_data,
									   kPF_MaskSuite, 
									   kPF_MaskSuiteVersion1,
									   0));

		}
	}

	return err;
}


static PF_Err 
Render (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*outputP )
{
PF_Err 				err = PF_Err_NONE;

	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	PF_EffectWorld		*inputP 		= &params[0]->u.ld;
	PF_EffectWorld		*outputPP 		= outputP;


	A_long				mask_idL	= params[SKELETON_Flood_Seed]->u.path_d.path_id;
	A_long				threshold	= params[SKELETON_BG]->u.sd.value;
	A_long				numPath		= 0;
	PF_PathOutlinePtr	maskP		= 0;
	PF_PathVertex		vertexP;		
	PF_MaskSuite1		*msP		= NULL;
	PF_Boolean			openB 		= TRUE,
						deepB		= PF_WORLD_IS_DEEP(outputP);
	A_long				linesL		= outputP->height;

	ERR(AEFX_AcquireSuite(in_data,
						  out_data,
						  kPF_MaskSuite,
						  kPF_MaskSuiteVersion1,
						  0,
						  reinterpret_cast<void**>(&msP)));

	ERR(suites.WorldTransformSuite1()->copy(in_data->effect_ref,inputP,outputP,NULL,NULL));
	ERR(suites.PathQuerySuite1()->PF_CheckoutPath(	in_data->effect_ref, 
														mask_idL,
														in_data->current_time,
														in_data->time_step,
														in_data->time_scale,
														&maskP));
	if (mask_idL&&maskP!=0){
		ERR(suites.PathDataSuite1()->PF_PathNumSegments(in_data->effect_ref, maskP,&numPath));
		A_long this_x,this_y;
			for(int ii=0;ii<=numPath;ii++){
				ERR(suites.PathDataSuite1()->PF_PathVertexInfo(	in_data->effect_ref, maskP,ii,&vertexP));
					this_x = (A_long)(vertexP.x+0.5);
					this_y = (A_long)(vertexP.y+0.5);
					if(IsPosValid(this_x,this_y,outputPP->width,outputPP->height)){
						fill(*outputPP,this_x,this_y,threshold);
					}
			}

	}
	ERR(suites.PathQuerySuite1()->PF_CheckinPath(	in_data->effect_ref, 
														mask_idL,
														FALSE, 
														maskP));
	if (msP){
		ERR(AEFX_ReleaseSuite(in_data,
							   out_data,
							   kPF_MaskSuite, 
							   kPF_MaskSuiteVersion1,
							   0));
	}
	return err;
}


DllExport	
PF_Err 
EntryPointFunc (
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try {

		switch (cmd) {
			case PF_Cmd_ABOUT:
				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:
				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:
				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_RENDER:
				err = Render(	in_data,
								out_data,
								params,
								output);
				break;
			case PF_Cmd_USER_CHANGED_PARAM:
				err = UserChangedParam(	in_data,
										out_data,
										params,
										output,
										reinterpret_cast<const PF_UserChangedParamExtra *>(extra));
				break;
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

