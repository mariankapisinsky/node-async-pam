{
  'targets': [
    {
      'target_name': 'auth_pam',
      'sources': [ 'src/auth-pam.c' ],
      'libraries': ['-lpam']
    }
  ]
}
